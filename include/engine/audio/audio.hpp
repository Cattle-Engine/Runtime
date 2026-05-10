#pragma once 

#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "engine/common/fs/vfs.hpp"

namespace CE::Core::Audio {
    enum class AudioType {
        SFX,
        Music,
        Error
    };

    struct AudioFilter {
        bool Enabled = false;

        enum class Type {
            LowPass,
            HighPass
        } Type = Type::LowPass;

        float CutoffHz = 1000.0f;

        // internal state
        int SampleRate = 0;
        int Channels = 0;
        float Alpha = 0.0f;
        std::vector<float> PrevIn;
        std::vector<float> PrevOut;
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

        AudioFilter Filter;
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
            void ApplyDSP(PlayingSound& sound);

        private:
            static void SDLCALL TrackCookedDSP(void* userdata, MIX_Track* track, const SDL_AudioSpec* spec, float* pcm, int samples);
            VFS::VFS& mVFS;
            int mInstanceID;
            MIX_Mixer* mMixer = nullptr;
            SDL_AudioSpec mMixerSpec = {};
            std::vector<MIX_Track*> mTracks;

            std::mutex mDSPMutex;
            std::unordered_map<MIX_Track*, AudioFilter> mTrackFilters;
    };
}
