#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

#include <algorithm>
#include <optional>

namespace CE::Core::Audio {
    namespace {
        struct VoiceCandidate {
            MIX_Track* Track = nullptr;
            uint64_t PlayOrder = 0;
        };
    }

    PlayingSound AudioSystem::CreateSoundInstance(const AudioClip& clip) {
        PlayingSound sound;
        sound.Clip = &clip;
        sound.Bus = DefaultBusName(clip.Type);

        if (!mMixer) {
            CE::Log(LogLevel::Error, "[Audio {}] Can't create sound instance: mixer not initialized.", mInstanceID);
            return sound;
        }

        if (!clip.Audio) {
            CE::Log(LogLevel::Error, "[Audio {}] Can't create sound instance: clip has no audio: {}", mInstanceID, clip.Path);
            return sound;
        }

        MIX_Track* track = MIX_CreateTrack(mMixer);
        if (!track) {
            CE::Log(LogLevel::Error, "[Audio {}] Failed to create track: {}", mInstanceID, SDL_GetError());
            return sound;
        }

        if (!MIX_SetTrackAudio(track, clip.Audio)) {
            CE::Log(LogLevel::Error, "[Audio {}] Failed to set track audio: {}", mInstanceID, SDL_GetError());
            MIX_DestroyTrack(track);
            return sound;
        }

        const char* tag = (clip.Type == AudioType::Music) ? "music" : "sfx";
        MIX_TagTrack(track, tag);

        mTracks.push_back(track);
        sound.Track = track;

        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            TrackState& state = mTrackStates[track];
            state.Type = clip.Type;
            state.Volume = sound.Volume;
            state.Bus = sound.Bus;
            state.Effects.clear();
            state.PlayOrder = 0;
        }
        return sound;
    }

    void AudioSystem::PlaySound(PlayingSound& sound) {
        if (!sound.Clip) {
            CE::Log(LogLevel::Error, "[Audio {}] PlaySound called with null clip.", mInstanceID);
            return;
        }

        if (!mMixer) {
            CE::Log(LogLevel::Error, "[Audio {}] PlaySound called without an audio mixer.", mInstanceID);
            return;
        }

        if (!sound.Track) {
            PlayingSound created = CreateSoundInstance(*sound.Clip);
            sound.Track = created.Track;
            if (!sound.Track) {
                return;
            }
            if (sound.Bus.empty()) {
                sound.Bus = created.Bus;
            }
        } else if (sound.Bus.empty()) {
            sound.Bus = DefaultBusName(sound.Clip->Type);
        }

        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            TrackState& state = mTrackStates[sound.Track];
            state.Type = sound.Clip->Type;
            state.Volume = sound.Volume;
            state.Bus = sound.Bus;
        }

        if (!EnforceVoiceLimits(sound.Track, sound.Bus)) {
            sound.IsPlaying = false;
            return;
        }

        UpdateTrackGain(sound.Track);
        ApplyDSP(sound);

        const Sint64 start_ms = (sound.PositionSeconds > 0.0f)
            ? static_cast<Sint64>(sound.PositionSeconds * 1000.0f)
            : 0;

        SDL_PropertiesID opts = 0;
        if (start_ms > 0 || (sound.Clip->Type == AudioType::Music)) {
            opts = SDL_CreateProperties();
            if (opts) {
                if (start_ms > 0) {
                    SDL_SetNumberProperty(opts, MIX_PROP_PLAY_START_MILLISECOND_NUMBER, start_ms);
                }
                if (sound.Clip->Type == AudioType::Music) {
                    SDL_SetNumberProperty(opts, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
                }
            } else {
                CE::Log(LogLevel::Warn, "[Audio {}] Failed to create playback properties: {}", mInstanceID, SDL_GetError());
            }
        }

        const bool ok = MIX_PlayTrack(sound.Track, opts);
        if (opts) {
            SDL_DestroyProperties(opts);
        }

        if (!ok) {
            CE::Log(LogLevel::Error, "[Audio {}] Failed to play track: {}", mInstanceID, SDL_GetError());
            sound.IsPlaying = false;
            return;
        }

        sound.IsPlaying = true;
        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            auto it = mTrackStates.find(sound.Track);
            if (it != mTrackStates.end()) {
                it->second.PlayOrder = ++mPlayCounter;
            }
        }
    }

    void AudioSystem::SeekSound(PlayingSound& sound, float position_seconds) {
        if (!sound.Track) {
            return;
        }
        if (position_seconds < 0.0f) {
            position_seconds = 0.0f;
        }

        const Sint64 ms = static_cast<Sint64>(position_seconds * 1000.0f);
        const Sint64 frames = MIX_TrackMSToFrames(sound.Track, ms);
        if (frames < 0) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to convert ms->frames for seek: {}", mInstanceID, SDL_GetError());
            return;
        }

        if (!MIX_SetTrackPlaybackPosition(sound.Track, frames)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to seek track: {}", mInstanceID, SDL_GetError());
            return;
        }

        sound.PositionSeconds = position_seconds;
    }

    void AudioSystem::PauseSound(PlayingSound& sound) {
        if (!sound.Track) {
            sound.IsPlaying = false;
            return;
        }

        if (!MIX_PauseTrack(sound.Track)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to pause track: {}", mInstanceID, SDL_GetError());
            return;
        }

        sound.IsPlaying = false;
    }

    void AudioSystem::ResumeSound(PlayingSound& sound) {
        if (!sound.Track) {
            sound.IsPlaying = false;
            return;
        }

        if (!MIX_ResumeTrack(sound.Track)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to resume track: {}", mInstanceID, SDL_GetError());
            sound.IsPlaying = false;
            return;
        }

        sound.IsPlaying = MIX_TrackPlaying(sound.Track);
    }

    void AudioSystem::StopSound(PlayingSound& sound) {
        if (!sound.Track) {
            sound.IsPlaying = false;
            return;
        }
        if (!MIX_StopTrack(sound.Track, 0)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to stop track: {}", mInstanceID, SDL_GetError());
        }
        sound.IsPlaying = false;
    }

    void AudioSystem::StopAll() {
        for (MIX_Track* track : mTracks) {
            MIX_StopTrack(track, 0);
        }
    }

    void AudioSystem::SetMasterVolume(float v) {
        mMasterVolume = std::clamp(v, 0.0f, 1.0f);
        UpdateAllTrackGains();
    }

    void AudioSystem::SetSFXVolume(float v) {
        mSFXVolume = std::clamp(v, 0.0f, 1.0f);
        UpdateAllTrackGains();
    }

    void AudioSystem::SetMusicVolume(float v) {
        mMusicVolume = std::clamp(v, 0.0f, 1.0f);
        UpdateAllTrackGains();
    }

    void AudioSystem::SetBusVolume(const std::string& bus, float v) {
        const std::string resolved = bus.empty() ? std::string(DefaultBusName(AudioType::SFX)) : bus;
        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            mBusVolumes[resolved] = std::clamp(v, 0.0f, 1.0f);
        }
        UpdateAllTrackGains();
    }

    void AudioSystem::SetGlobalVoiceLimit(size_t max_voices) {
        std::lock_guard<std::mutex> lock(mDSPMutex);
        mGlobalVoiceLimit = max_voices;
    }

    void AudioSystem::SetBusVoiceLimit(const std::string& bus, size_t max_voices) {
        if (bus.empty()) {
            return;
        }

        std::lock_guard<std::mutex> lock(mDSPMutex);
        mBusVoiceLimits[bus] = max_voices;
    }

    float AudioSystem::ComputeTrackGain(const TrackState& state) const {
        const float base = std::clamp(static_cast<float>(state.Volume) / 128.0f, 0.0f, 1.0f);
        const float category = (state.Type == AudioType::Music) ? mMusicVolume : mSFXVolume;
        float bus_gain = 1.0f;
        const auto bus_it = mBusVolumes.find(state.Bus);
        if (bus_it != mBusVolumes.end()) {
            bus_gain = bus_it->second;
        }
        return std::clamp(base * category * bus_gain * mMasterVolume, 0.0f, 1.0f);
    }

    void AudioSystem::UpdateTrackGain(MIX_Track* track) {
        if (!track) {
            return;
        }

        float gain = 1.0f;
        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            const auto it = mTrackStates.find(track);
            if (it == mTrackStates.end()) {
                return;
            }
            gain = ComputeTrackGain(it->second);
        }

        if (!MIX_SetTrackGain(track, gain)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to set track gain: {}", mInstanceID, SDL_GetError());
        }
    }

    void AudioSystem::UpdateAllTrackGains() {
        std::vector<std::pair<MIX_Track*, float>> gains;
        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            gains.reserve(mTrackStates.size());
            for (const auto& [track, state] : mTrackStates) {
                gains.emplace_back(track, ComputeTrackGain(state));
            }
        }

        for (const auto& [track, gain] : gains) {
            if (!track) {
                continue;
            }
            if (!MIX_SetTrackGain(track, gain)) {
                CE::Log(LogLevel::Warn, "[Audio {}] Failed to update track gain: {}", mInstanceID, SDL_GetError());
            }
        }
    }

    bool AudioSystem::EnforceVoiceLimits(MIX_Track* requested_track, const std::string& bus) {
        std::optional<VoiceCandidate> oldest_global;
        std::optional<VoiceCandidate> oldest_bus;
        size_t active_global = 0;
        size_t active_bus = 0;
        size_t global_limit = 0;
        size_t bus_limit = 0;

        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            global_limit = mGlobalVoiceLimit;

            const auto bus_limit_it = mBusVoiceLimits.find(bus);
            if (bus_limit_it != mBusVoiceLimits.end()) {
                bus_limit = bus_limit_it->second;
            }

            for (const auto& [track, state] : mTrackStates) {
                if (!track || track == requested_track || !IsTrackActive(track)) {
                    continue;
                }

                ++active_global;
                if (!oldest_global || state.PlayOrder < oldest_global->PlayOrder) {
                    oldest_global = VoiceCandidate{track, state.PlayOrder};
                }

                if (state.Bus == bus) {
                    ++active_bus;
                    if (!oldest_bus || state.PlayOrder < oldest_bus->PlayOrder) {
                        oldest_bus = VoiceCandidate{track, state.PlayOrder};
                    }
                }
            }
        }

        if (bus_limit > 0 && active_bus >= bus_limit) {
            if (!oldest_bus || !MIX_StopTrack(oldest_bus->Track, 0)) {
                CE::Log(LogLevel::Warn, "[Audio {}] Voice limit hit for bus '{}' and no voice could be stolen: {}", mInstanceID, bus, SDL_GetError());
                return false;
            }
        } else if (global_limit > 0 && active_global >= global_limit) {
            if (!oldest_global || !MIX_StopTrack(oldest_global->Track, 0)) {
                CE::Log(LogLevel::Warn, "[Audio {}] Global voice limit hit and no voice could be stolen: {}", mInstanceID, SDL_GetError());
                return false;
            }
        }

        return true;
    }

    const char* AudioSystem::DefaultBusName(AudioType type) {
        switch (type) {
            case AudioType::Music:
                return "music";
            case AudioType::SFX:
                return "sfx";
            default:
                return "default";
        }
    }

    bool AudioSystem::IsTrackActive(MIX_Track* track) {
        return track && (MIX_TrackPlaying(track) || MIX_TrackPaused(track));
    }
}
