#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "engine/audio/audio.hpp"

namespace CE::Assets::Audio {
    class AudioManager {
        public:
        AudioManager(Core::Audio::AudioSystem& audio_system,
                    VFS::VFS& vfs, int instance_id);

        void LoadSound(const std::string& path, const std::string& name, Core::Audio::AudioType type);
        void UnloadSound(const std::string& name);

        uint32_t CreateSoundInstance(const std::string& name);
        bool IsPlaying(uint32_t handle) const;
        void DeleteSoundInstance(uint32_t handle);

        void PlaySound(uint32_t handle);
        void PauseSound(uint32_t handle);
        void ResumeSound(uint32_t handle);
        void SeekSound(uint32_t handle, float seconds);
        void StopSound(uint32_t handle);

        void AddEffect(uint32_t handle, std::string name,Core::Audio::AudioFilter effect);
        void RemoveEffect(uint32_t handle, std::string name);
        void ClearEffects(uint32_t handle);

        void SetSoundBus(uint32_t handle, const std::string& bus);
        std::string GetSoundBus(uint32_t handle) const;
        void SetBusVolume(const std::string& bus, float volume);
        void SetBusVoiceLimit(const std::string& bus, size_t limit);
        void SetSoundVolume(uint32_t handle, int volume);

        private:
        struct AMPlayingSoundInfo {
            uint32_t Id;
            Core::Audio::PlayingSound Sound;
            std::vector<Core::Audio::AudioFilter> Effects;
            std::string ClipName;
        };
        uint32_t NextHandleID = 0;
        int mInstanceID;
        VFS::VFS& mVFS;
        Core::Audio::AudioSystem& mAudioSys;
        std::unordered_map<std::string, Core::Audio::AudioClip*> mAudioCache;
        std::unordered_map<uint32_t, AMPlayingSoundInfo> mPlayingSounds;
        
        AMPlayingSoundInfo* GetSoundInfo(uint32_t handle);
    };
}