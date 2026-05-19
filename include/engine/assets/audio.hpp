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
        bool IsPlaying(uint32_t handle);
        void DeleteSoundInstance(uint32_t handle);

        void PlaySound(uint32_t handle);
        void PauseSound(uint32_t handle);
        void ResumeSound(uint32_t handle);
        void SeekSound(uint32_t handle, float seconds);
        void StopSound(uint32_t handle);
        void StopAll();
        void PauseAll();
        void ResumeAll();
        void AddEffect(uint32_t handle, std::string name,Core::Audio::AudioFilter effect);
        void RemoveEffect(uint32_t handle, std::string name);
        void ClearEffects(uint32_t handle);

        void SetSoundBus(uint32_t handle, const std::string& bus);
        std::string GetSoundBus(uint32_t handle);
        void SetBusVolume(const std::string& bus, float volume);
        void SetBusVoiceLimit(const std::string& bus, size_t limit);
        void SetSoundVolume(uint32_t handle, int volume);
        void SetMasterVolume(float volume);
        void SetMusicVolume(float volume);
        void SetSFXVolume(float volume);

        void SetSoundMuted(uint32_t handle, bool muted);
        void SetSoundGain(uint32_t handle, float gain);

        size_t Debug_ActiveVoices() const;
        void Debug_KillOldestVoice();

        struct DebugPlayingSound {
            uint32_t Handle = 0;
            std::string ClipName;
            std::string Bus;
            int Volume = 128;
            bool IsPlaying = false;
            size_t EffectCount = 0;
            float PositionSeconds = 0.0f;
            float DurationSeconds = 0.0f;
            bool Muted = false;
            float Gain = 1.0f;
        };
        size_t Debug_CachedClipsCount() const;
        std::vector<DebugPlayingSound> Debug_PlayingSoundsSnapshot() const;

        private:
        struct AMPlayingSoundInfo {
            uint32_t Id;
            Core::Audio::PlayingSound Sound;
            struct NamedEffect {
                std::string Name;
                Core::Audio::AudioFilter Effect;
            };
            std::vector<NamedEffect> Effects;
            std::string ClipName;
            bool Muted = false;
            float Gain = 1.0f;
            float PreviousVolume = 1.0f;
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
