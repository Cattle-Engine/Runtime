#include "engine/assets/audio.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Audio {
    AudioManager::AudioManager(Core::Audio::AudioSystem& audio_system,
                    VFS::VFS& vfs, int instance_id) : mVFS(vfs), mAudioSys(audio_system)
    {
        mInstanceID = instance_id;
    }

    AudioManager::AMPlayingSoundInfo* AudioManager::GetSoundInfo(uint32_t handle) {
        auto it = mPlayingSounds.find(handle);
        if (it == mPlayingSounds.end()) {
            CE::Log(LogLevel::Error, "[Audio Manager {}] Use after free detected: {}", mInstanceID, handle);
            return nullptr;
        } else {
            return &it->second;
        }
    }

    void AudioManager::LoadSound(const std::string& path, const std::string& name, Core::Audio::AudioType type) {
        if (!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error, "[Audio Manager {}] Audio file does not exist: {}", mInstanceID, path);
            return;
        }
        auto audio_clip = mAudioSys.LoadSound(path, type);
        if (audio_clip == nullptr) {
            CE::Log(LogLevel::Error, "[Audio Manager {}] Failed to load audio file", mInstanceID);
            return;
        }
        mAudioCache[name] = audio_clip;
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
        CE::Log(LogLevel::Error, "[Audio Manager {}] Tried to use a missing or unloaded asset: {}", mInstanceID, name);
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
}
