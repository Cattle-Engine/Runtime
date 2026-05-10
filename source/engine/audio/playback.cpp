#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

#include <algorithm>

namespace CE::Core::Audio {
    PlayingSound AudioSystem::CreateSoundInstance(const AudioClip& clip) {
        PlayingSound sound;
        sound.Clip = &clip;

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
        }

        const float gain = std::clamp(static_cast<float>(sound.Volume) / 128.0f, 0.0f, 1.0f);
        if (!MIX_SetTrackGain(sound.Track, gain)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to set track gain: {}", mInstanceID, SDL_GetError());
        }

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
}
