#pragma once 

#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <cstdint>
#include <vector>

#include "engine/common/fs/vfs.hpp"

namespace CE::Core::Audio {
    enum class AudioType {
        SFX,
        Music,
        Error
    };
    
    struct AudioClip {
        MIX_Audio* Audio = nullptr;
        AudioType Type = AudioType::Error;
        std::string Path;
        bool IsError = false;
        bool IsLoaded = false;
    };

    struct PlayingSound {
        MIX_Track* Track = nullptr;
        const AudioClip* Clip = nullptr;
        float PositionSeconds = 0.0f;
        int Volume = 128; // 0..128 maps to gain 0..1
        bool IsPlaying = false;
    };

    struct AudioDeviceInfo {
        uint32_t Id;
        std::string Name;
    };
    
    class AudioSystem {
        public:
            AudioSystem(VFS::VFS& vfs, int instanceid, uint32_t device_id, bool stero);
            ~AudioSystem();

            std::vector<AudioDeviceInfo> ListAudioDevices();
            void SetAudioDevice(uint32_t device_id, bool stero);

            AudioClip* LoadSound(const std::string& path, const AudioType type);
            void DestroySound(AudioClip* clip);

            PlayingSound CreateSoundInstance(const AudioClip& clip);
            void PlaySound(PlayingSound& sound);
            void SeekSound(PlayingSound& sound, float position_seconds);
            void StopSound(PlayingSound& sound);
            void StopAll();

        private:
            VFS::VFS& mVFS;
            int mInstanceID;
            MIX_Mixer* mMixer = nullptr;
            SDL_AudioSpec mMixerSpec = {};
            std::vector<MIX_Track*> mTracks;
    };
}
