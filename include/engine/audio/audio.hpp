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
        AudioType Type;
        std::string Path;
        bool IsError = false;
        bool IsLoaded = false;
    };

    struct PlayingSound {
        MIX_Track* Track = nullptr;
        AudioClip& Clip;
        float Position;
        int Volume;
        bool IsPlaying = false;
    };

    struct AudioDeviceInfo {
        uint32_t Id;
        std::string Name;
    };
    
    class AudioSystem {
        public:
            AudioSystem(VFS::VFS& vfs, int instanceid, uint32_t device_id, bool stero);

            std::vector<AudioDeviceInfo> ListAudioDevices();
            void SetAudioDevice(uint32_t device_id, bool stero);

            AudioClip* LoadSound(const std::string& path, const AudioType type);
            void DestroySound(AudioClip* clip);

            PlayingSound CreateSoundInstance(const AudioClip& clip);
            void PlaySound(const PlayingSound& sound);
            void SeekSound(const PlayingSound& sound, float position);
            void StopSound(const PlayingSound& sound);
            void StopAll();

        private:
            VFS::VFS& mVFS;
            int mInstanceID;
            MIX_Mixer* mMixer;
    };
}