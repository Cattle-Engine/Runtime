#include "engine/audio/audio_resource_manager.hpp"

#include <algorithm>
#include <utility>

#include "engine/common/tracelog.hpp"

namespace CE::Audio::Resources {
    AudioManager::AudioManager(Core::Audio::AudioSystem& audio_system, Common::FS::VFS::VFS& vfs, int instance_id)
        : mVFS(vfs), mAudioSys(audio_system) {
        mInstanceID = instance_id;
    }

    AudioManager::AMPlayingSoundInfo* AudioManager::GetSoundInfo(PlayingAudioHandle handle) {
        auto it = mPlayingSounds.find(handle.id);
        if (it == mPlayingSounds.end()) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Use after free or invalid handle: {}", mInstanceID, handle.id);
            return nullptr;
        }

        return &it->second;
    }

    AudioHandle AudioManager::LoadSound(const std::string& path, Core::Audio::AudioType type) {
        if (!mVFS.FileExists(path.c_str())) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Audio file does not exist: {}", mInstanceID, path);
            return {};
        }

        auto audio_clip = mAudioSys.LoadSound(path, type);
        if (audio_clip == nullptr) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Failed to load audio file", mInstanceID);
            return {};
        }

        const AudioHandle handle{NextAudioHandleID++};
        mAudioCache.emplace(handle.id, audio_clip);
        return handle;
    }

    void AudioManager::UnloadSound(AudioHandle handle) {
        auto it = mAudioCache.find(handle.id);
        if (it == mAudioCache.end()) {
            CE_LOG(LogLevel::Warn, "[Audio Manager {}] Tried to unload missing asset handle: {}", mInstanceID,
                   handle.id);
            return;
        }

        std::vector<PlayingAudioHandle> handles_to_delete;
        handles_to_delete.reserve(mPlayingSounds.size());

        for (const auto& [playing_handle, info] : mPlayingSounds) {
            if (info.Sound.Clip == it->second) {
                handles_to_delete.push_back({playing_handle});
            }
        }

        for (PlayingAudioHandle playing_handle : handles_to_delete) {
            DeleteSoundInstance(playing_handle);
        }

        mAudioSys.DestroySound(it->second);
        mAudioCache.erase(it);
    }

    PlayingAudioHandle AudioManager::CreateSoundInstance(AudioHandle handle) {
        auto it = mAudioCache.find(handle.id);
        if (it != mAudioCache.end()) {
            AMPlayingSoundInfo info = {};
            info.Sound = mAudioSys.CreateSoundInstance(*it->second);
            info.Handle = {NextPlayingAudioHandleID++};
            mPlayingSounds[info.Handle.id] = info;
            return info.Handle;
        }

        CE_LOG(LogLevel::Error, "[Audio Manager {}] Tried to use missing or unloaded asset handle: {}", mInstanceID,
               handle.id);
        return {};
    }

    void AudioManager::PlaySound(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.PlaySound(info->Sound);
    }

    void AudioManager::PauseSound(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.PauseSound(info->Sound);
    }

    void AudioManager::ResumeSound(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.ResumeSound(info->Sound);
    }

    void AudioManager::SeekSound(PlayingAudioHandle handle, float seconds) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.SeekSound(info->Sound, seconds);
    }

    void AudioManager::StopSound(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.StopSound(info->Sound);
    }

    bool AudioManager::IsPlaying(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return false;
        }

        return info->Sound.IsPlaying;
    }

    void AudioManager::SetSoundBus(PlayingAudioHandle handle, const std::string& bus) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        info->Sound.Bus = bus;
        mAudioSys.UpdateSound(info->Sound);
    }

    std::string AudioManager::GetSoundBus(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return {};
        }

        return info->Sound.Bus;
    }

    void AudioManager::SetBusVolume(const std::string& bus, float volume) {
        mAudioSys.SetBusVolume(bus, volume);
    }

    void AudioManager::SetBusVoiceLimit(const std::string& bus, size_t limit) {
        mAudioSys.SetBusVoiceLimit(bus, limit);
    }

    void AudioManager::SetSoundVolume(PlayingAudioHandle handle, int volume) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        info->Sound.Volume = volume;
        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::SetMasterVolume(float volume) {
        mAudioSys.SetMasterVolume(volume);
    }

    void AudioManager::SetMusicVolume(float volume) {
        mAudioSys.SetMusicVolume(volume);
    }

    void AudioManager::SetSFXVolume(float volume) {
        mAudioSys.SetSFXVolume(volume);
    }

    void AudioManager::SetSoundLabel(PlayingAudioHandle handle, std::string label) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        info->Label = std::move(label);
    }

    std::string AudioManager::GetSoundLabel(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return {};
        }

        return info->Label;
    }

    void AudioManager::AddEffect(PlayingAudioHandle handle, std::string name, Core::Audio::AudioEffect effect) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        bool replaced = false;
        for (auto& named : info->Effects) {
            if (named.Name == name) {
                named.Effect = effect;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            info->Effects.push_back({std::move(name), effect});
        }

        info->Sound.Effects.clear();
        info->Sound.Effects.reserve(info->Effects.size());

        for (const auto& named : info->Effects) {
            info->Sound.Effects.push_back(named.Effect);
        }

        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::RemoveEffect(PlayingAudioHandle handle, std::string name) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        auto it = info->Effects.begin();
        for (; it != info->Effects.end(); ++it) {
            if (it->Name == name) {
                info->Effects.erase(it);
                break;
            }
        }

        info->Sound.Effects.clear();
        info->Sound.Effects.reserve(info->Effects.size());

        for (const auto& named : info->Effects) {
            info->Sound.Effects.push_back(named.Effect);
        }

        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::ClearEffects(PlayingAudioHandle handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        info->Effects.clear();
        info->Sound.Effects.clear();
        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::DeleteSoundInstance(PlayingAudioHandle handle) {
        auto it = mPlayingSounds.find(handle.id);
        if (it == mPlayingSounds.end()) {
            CE_LOG(LogLevel::Warn, "[Audio Manager {}] Tried to delete invalid handle: {}", mInstanceID, handle.id);
            return;
        }

        AMPlayingSoundInfo& info = it->second;
        mAudioSys.StopSound(info.Sound);
        info.Effects.clear();
        mAudioSys.DestroySoundInstance(info.Sound);
        mPlayingSounds.erase(it);
    }

    size_t AudioManager::Debug_CachedClipsCount() const {
        return mAudioCache.size();
    }

    std::vector<AudioManager::DebugPlayingSound> AudioManager::Debug_PlayingSoundsSnapshot() const {
        std::vector<DebugPlayingSound> snapshot;
        snapshot.reserve(mPlayingSounds.size());

        for (const auto& [handle, info] : mPlayingSounds) {
            DebugPlayingSound row;
            row.Handle = {handle};
            row.Label = info.Label;
            row.Bus = info.Sound.Bus;
            row.Volume = info.Sound.Volume;
            row.IsPlaying = info.Sound.IsPlaying;
            row.EffectCount = info.Effects.size();
            snapshot.push_back(std::move(row));
        }

        return snapshot;
    }

    void AudioManager::StopAll() {
        mAudioSys.StopAll();
    }

    void AudioManager::PauseAll() {
        for (auto& [handle, info] : mPlayingSounds) {
            mAudioSys.PauseSound(info.Sound);
        }
    }

    void AudioManager::ResumeAll() {
        for (auto& [handle, info] : mPlayingSounds) {
            mAudioSys.ResumeSound(info.Sound);
        }
    }

    void AudioManager::SetSoundMuted(PlayingAudioHandle handle, bool muted) {
        auto* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        if (muted && !info->Muted) {
            info->PreviousVolume = info->Sound.Volume;
            info->Sound.Volume = 0;
        } else if (!muted && info->Muted) {
            info->Sound.Volume = info->PreviousVolume;
        }

        info->Muted = muted;
        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::SetSoundGain(PlayingAudioHandle handle, float gain) {
        auto* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        gain = std::clamp(gain, 0.0f, 1.0f);
        info->Gain = gain;

        if (info->Muted) {
            return;
        }

        info->Sound.Volume = static_cast<int>(gain * 128.0f);
        mAudioSys.UpdateSound(info->Sound);
    }

    size_t AudioManager::Debug_ActiveVoices() const {
        return mPlayingSounds.size();
    }

    void AudioManager::Debug_KillOldestVoice() {
        uint32_t oldest = 0;
        bool found = false;

        for (auto& [id, info] : mPlayingSounds) {
            if (!found || id < oldest) {
                oldest = id;
                found = true;
            }
        }

        if (found) {
            mAudioSys.StopSound(mPlayingSounds[oldest].Sound);
        }
    }

    bool AudioManager::SetAudioDevice(uint32_t device_id, bool stereo) {
        struct SavedSound {
            uint32_t Handle = 0;
            Core::Audio::PlayingSound Sound;
            bool WasPlaying = false;
            bool WasPaused = false;
            float PositionSeconds = 0.0f;
        };

        std::vector<SavedSound> saved_sounds;
        saved_sounds.reserve(mPlayingSounds.size());

        // Snapshot every sound before the old mixer destroys its tracks.
        for (auto& [handle, info] : mPlayingSounds) {
            SavedSound saved;
            saved.Handle = handle;
            saved.Sound = info.Sound;
            saved.PositionSeconds = info.Sound.PositionSeconds;

            MIX_Track* track = info.Sound.Track;
            if (track) {
                saved.WasPaused = MIX_TrackPaused(track);
                saved.WasPlaying = MIX_TrackPlaying(track) || saved.WasPaused;

                const Sint64 frames = MIX_GetTrackPlaybackPosition(track);
                if (frames >= 0) {
                    const Sint64 milliseconds = MIX_TrackFramesToMS(track, frames);
                    if (milliseconds >= 0) {
                        saved.PositionSeconds =
                            static_cast<float>(milliseconds) / 1000.0f;
                    }
                }
            }

            saved.Sound.Track = nullptr;
            saved.Sound.PositionSeconds = saved.PositionSeconds;
            saved.Sound.IsPlaying = saved.WasPlaying;

            saved_sounds.push_back(std::move(saved));
        }

        if (!mAudioSys.SetAudioDevice(device_id, stereo)) {
            return false;
        }

        // Rebuild the mixer-specific tracks while keeping the manager's handles.
        for (auto& saved : saved_sounds) {
            auto it = mPlayingSounds.find(saved.Handle);
            if (it == mPlayingSounds.end() || !saved.Sound.Clip) {
                continue;
            }

            AMPlayingSoundInfo& info = it->second;

            Core::Audio::PlayingSound rebuilt =
                mAudioSys.CreateSoundInstance(*saved.Sound.Clip);

            if (!rebuilt.Track) {
                CE_LOG(LogLevel::Error,
                    "[Audio Manager {}] Failed to recreate sound instance {} "
                    "after changing audio device",
                    mInstanceID, saved.Handle);
                info.Sound = std::move(rebuilt);
                continue;
            }

            // Restore the sound instance's logical state.
            rebuilt.Volume = saved.Sound.Volume;
            rebuilt.Bus = std::move(saved.Sound.Bus);
            rebuilt.Filter = std::move(saved.Sound.Filter);
            rebuilt.Effects = std::move(saved.Sound.Effects);
            rebuilt.PositionSeconds = saved.PositionSeconds;
            rebuilt.IsPlaying = false;

            info.Sound = std::move(rebuilt);

            // Reapply per-track gain, bus and DSP configuration.
            mAudioSys.UpdateSound(info.Sound);

            if (!saved.WasPlaying) {
                continue;
            }

            // Start the track, then seek. Seeking before PlaySound() can be
            // ineffective because starting a track resets its start position.
            mAudioSys.PlaySound(info.Sound);

            if (saved.PositionSeconds > 0.0f) {
                mAudioSys.SeekSound(info.Sound, saved.PositionSeconds);
            }

            if (saved.WasPaused) {
                mAudioSys.PauseSound(info.Sound);
            }
        }

        return true;
    }
} // namespace CE::Audio::Resources