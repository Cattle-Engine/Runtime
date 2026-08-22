#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/audio/audio.hpp"

namespace CE::Audio::Resources {
    struct AudioHandle {
        uint32_t id = 0;

        explicit operator bool() const {
            return id != 0;
        }
    };

    struct PlayingAudioHandle {
        uint32_t id = 0;

        explicit operator bool() const {
            return id != 0;
        }
    };

    class AudioManager {
      public:
        AudioManager(Core::Audio::AudioSystem& audio_system, VFS::VFS& vfs, int instance_id);

        AudioHandle LoadSound(const std::string& path, Core::Audio::AudioType type);
        void UnloadSound(AudioHandle handle);

        PlayingAudioHandle CreateSoundInstance(AudioHandle handle);
        bool IsPlaying(PlayingAudioHandle handle);
        void DeleteSoundInstance(PlayingAudioHandle handle);

        void PlaySound(PlayingAudioHandle handle);
        void PauseSound(PlayingAudioHandle handle);
        void ResumeSound(PlayingAudioHandle handle);
        void SeekSound(PlayingAudioHandle handle, float seconds);
        void StopSound(PlayingAudioHandle handle);
        void StopAll();
        void PauseAll();
        void ResumeAll();
        void AddEffect(PlayingAudioHandle handle, std::string name, Core::Audio::AudioFilter effect);
        void RemoveEffect(PlayingAudioHandle handle, std::string name);
        void ClearEffects(PlayingAudioHandle handle);

        void SetSoundBus(PlayingAudioHandle handle, const std::string& bus);
        std::string GetSoundBus(PlayingAudioHandle handle);
        void SetBusVolume(const std::string& bus, float volume);
        void SetBusVoiceLimit(const std::string& bus, size_t limit);
        void SetSoundVolume(PlayingAudioHandle handle, int volume);
        void SetMasterVolume(float volume);
        void SetMusicVolume(float volume);
        void SetSFXVolume(float volume);

        void SetSoundMuted(PlayingAudioHandle handle, bool muted);
        void SetSoundGain(PlayingAudioHandle handle, float gain);

        size_t Debug_ActiveVoices() const;
        void Debug_KillOldestVoice();

        void SetSoundLabel(PlayingAudioHandle handle, std::string label);
        std::string GetSoundLabel(PlayingAudioHandle handle);

        struct DebugPlayingSound {
            PlayingAudioHandle Handle{};
            std::string Label;
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
            PlayingAudioHandle Handle;
            Core::Audio::PlayingSound Sound;
            struct NamedEffect {
                std::string Name;
                Core::Audio::AudioFilter Effect;
            };
            std::vector<NamedEffect> Effects;
            std::string Label;
            bool Muted = false;
            float Gain = 1.0f;
            float PreviousVolume = 1.0f;
        };
        uint32_t NextAudioHandleID = 1;
        uint32_t NextPlayingAudioHandleID = 1;
        int mInstanceID;
        VFS::VFS& mVFS;
        Core::Audio::AudioSystem& mAudioSys;
        std::unordered_map<uint32_t, Core::Audio::AudioClip*> mAudioCache;
        std::unordered_map<uint32_t, AMPlayingSoundInfo> mPlayingSounds;

        AMPlayingSoundInfo* GetSoundInfo(PlayingAudioHandle handle);
    };
} // namespace CE::Audio::Resources
