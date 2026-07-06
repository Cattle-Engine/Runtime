#include "engine/assets/audio.hpp"
#include "engine/common/tracelog.hpp"

#include <algorithm>

namespace CE::Assets::Audio {
    AudioManager::AudioManager(Core::Audio::AudioSystem& audio_system,
                    VFS::VFS& vfs, int instance_id) : mVFS(vfs), mAudioSys(audio_system)
    {
        mInstanceID = instance_id;
    }

    AudioManager::AMPlayingSoundInfo* AudioManager::GetSoundInfo(uint32_t handle) {
        auto it = mPlayingSounds.find(handle);
        if (it == mPlayingSounds.end()) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Use after free or invalid handle: {}", mInstanceID, handle);
            return nullptr;
        } else {
            return &it->second;
        }
    }

    void AudioManager::LoadSound(const std::string& path, const std::string& name, Core::Audio::AudioType type) {
        if (!mVFS.FileExists(path.c_str())) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Audio file does not exist: {}", mInstanceID, path);
            return;
        }
        auto audio_clip = mAudioSys.LoadSound(path, type);
        if (audio_clip == nullptr) {
            CE_LOG(LogLevel::Error, "[Audio Manager {}] Failed to load audio file", mInstanceID);
            return;
        }
        mAudioCache[name] = audio_clip;
    }

    void AudioManager::UnloadSound(const std::string& name) {
        auto it = mAudioCache.find(name);
        if (it == mAudioCache.end()) {
            CE_LOG(LogLevel::Warn, "[Audio Manager {}] Tried to unload missing asset: {}", mInstanceID, name);
            return;
        }

        std::vector<uint32_t> handles_to_delete;
        handles_to_delete.reserve(mPlayingSounds.size());
        for (const auto& [handle, info] : mPlayingSounds) {
            if (info.ClipName == name) {
                handles_to_delete.push_back(handle);
            }
        }
        for (uint32_t handle : handles_to_delete) {
            DeleteSoundInstance(handle);
        }

        mAudioSys.DestroySound(it->second);
        mAudioCache.erase(it);
    }

   uint32_t AudioManager::CreateSoundInstance(const std::string& name) {
        auto it = mAudioCache.find(name);
        if (it != mAudioCache.end()) {
            AMPlayingSoundInfo info = {};
            info.ClipName = name;
            info.Sound = mAudioSys.CreateSoundInstance(*it->second);
            info.Id = NextHandleID++;
            mPlayingSounds[info.Id] = info;
            return info.Id;
        }
        CE_LOG(LogLevel::Error, "[Audio Manager {}] Tried to use a missing or unloaded asset: {}", mInstanceID, name);
        return 0;
   }

   void AudioManager::PlaySound(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        mAudioSys.PlaySound(info->Sound);
   }

   void AudioManager::PauseSound(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }
        mAudioSys.PauseSound(info->Sound);
   }

   void AudioManager::ResumeSound(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }
        mAudioSys.ResumeSound(info->Sound);
   }

   void AudioManager::SeekSound(uint32_t handle, float seconds) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if(!info) {
            return;
        }
        mAudioSys.SeekSound(info->Sound, seconds);
   }

   void AudioManager::StopSound(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if(!info) {
            return;
        }
        mAudioSys.StopSound(info->Sound);
   }

   bool AudioManager::IsPlaying(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return false;
        }
        return info->Sound.IsPlaying;
   }

   void AudioManager::SetSoundBus(uint32_t handle, const std::string& bus) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if(!info) {
            return;
        }
        info->Sound.Bus = bus;
        mAudioSys.UpdateSound(info->Sound);
   }

   std::string AudioManager::GetSoundBus(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if(!info) {
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

   void AudioManager::SetSoundVolume(uint32_t handle, int volume) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if(!info) {
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

    void AudioManager::AddEffect(uint32_t handle, std::string name, Core::Audio::AudioFilter effect) {
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

    void AudioManager::RemoveEffect(uint32_t handle, std::string name) {
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

    void AudioManager::ClearEffects(uint32_t handle) {
        AMPlayingSoundInfo* info = GetSoundInfo(handle);
        if (!info) {
            return;
        }

        info->Effects.clear();
        info->Sound.Effects.clear();
        mAudioSys.UpdateSound(info->Sound);
    }

    void AudioManager::DeleteSoundInstance(uint32_t handle) {
        auto it = mPlayingSounds.find(handle);
        if (it == mPlayingSounds.end()) {
            CE_LOG(LogLevel::Warn,
                "[Audio Manager {}] Tried to delete invalid handle: {}",
                mInstanceID, handle);
            return;
        }

        AMPlayingSoundInfo& info = it->second;
        mAudioSys.StopSound(info.Sound);
        info.Effects.clear();
        mAudioSys.DestroySoundInstance(it->second.Sound);
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
            row.Handle = handle;
            row.ClipName = info.ClipName;
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

    void AudioManager::SetSoundMuted(uint32_t handle, bool muted) {
        auto* info = GetSoundInfo(handle);
        if (!info) return;

        if (muted && !info->Muted) {
            info->PreviousVolume = info->Sound.Volume;
            info->Sound.Volume = 0;
        }
        else if (!muted && info->Muted) {
            info->Sound.Volume = info->PreviousVolume;
        }

        info->Muted = muted;

        mAudioSys.UpdateSound(info->Sound);
    }

    void CE::Assets::Audio::AudioManager::SetSoundGain(uint32_t handle, float gain) {
        auto* info = GetSoundInfo(handle);
        if (!info) return;

        gain = std::clamp(gain, 0.0f, 1.0f);
        info->Gain = gain;

        if (info->Muted) return;

        info->Sound.Volume = static_cast<int>(gain * 128.0f);
        mAudioSys.UpdateSound(info->Sound);
    }

    size_t CE::Assets::Audio::AudioManager::Debug_ActiveVoices() const {
        return mPlayingSounds.size();
    }

    void CE::Assets::Audio::AudioManager::Debug_KillOldestVoice() {
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
}
