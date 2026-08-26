#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"

#include <SDL3_mixer/SDL_mixer.h>

namespace CE::Core::Audio {
    enum class AudioType { SFX, Music, Error };

    struct AudioEffect {
        bool Enabled = false;

        enum class Type { LowPass, HighPass, Reverb, Delay, Chorus } Kind = Type::LowPass;

        float CutoffHz = 1000.0f;
        float WetMix = 0.35f;
        float Feedback = 0.35f;
        float DelayMs = 180.0f;
        float DepthMs = 8.0f;
        float RateHz = 0.5f;
        float RoomSize = 0.6f;
        float Damping = 0.4f;

        // internal state
        int SampleRate = 0;
        int Channels = 0;
        float Alpha = 0.0f;
        size_t WriteFrame = 0;
        float LFOPhase = 0.0f;
        std::vector<float> PrevIn;
        std::vector<float> PrevOut;
        std::vector<float> DelayBuffer;
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
        std::string Bus;

        AudioEffect Filter;
        std::vector<AudioEffect> Effects;
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
        void PauseSound(PlayingSound& sound);
        void ResumeSound(PlayingSound& sound);
        void SeekSound(PlayingSound& sound, float position_seconds);
        void StopSound(PlayingSound& sound);
        void UpdateSound(PlayingSound& sound);
        void DestroySoundInstance(PlayingSound& sound);
        void StopAll();
        void SetMasterVolume(float v);
        void SetSFXVolume(float v);
        void SetMusicVolume(float v);
        void SetBusVolume(const std::string& bus, float v);
        void SetGlobalVoiceLimit(size_t max_voices);
        void SetBusVoiceLimit(const std::string& bus, size_t max_voices);
        void ApplyDSP(PlayingSound& sound);

      private:
        struct TrackState {
            AudioType Type = AudioType::Error;
            int Volume = 128;
            std::string Bus;
            std::vector<AudioEffect> Effects;
            uint64_t PlayOrder = 0;
        };

        static void SDLCALL TrackCookedDSP(void* userdata, MIX_Track* track, const SDL_AudioSpec* spec, float* pcm,
                                           int samples);
        float ComputeTrackGain(const TrackState& state) const;
        void UpdateTrackGain(MIX_Track* track);
        void UpdateAllTrackGains();
        std::vector<AudioEffect> BuildEffectChain(const PlayingSound& sound) const;
        bool EnforceVoiceLimits(MIX_Track* requested_track, const std::string& bus);
        static const char* DefaultBusName(AudioType type);
        static bool IsTrackActive(MIX_Track* track);
        float GetSoundPositionSeconds(MIX_Track* track);

        VFS::VFS& mVFS;
        int mInstanceID;
        MIX_Mixer* mMixer = nullptr;
        SDL_AudioSpec mMixerSpec = {};
        std::vector<MIX_Track*> mTracks;
        uint64_t mPlayCounter = 0;
        size_t mGlobalVoiceLimit = 0;
        float mMasterVolume = 1.0f;
        float mSFXVolume = 1.0f;
        float mMusicVolume = 1.0f;
        std::unordered_map<std::string, float> mBusVolumes;
        std::unordered_map<std::string, size_t> mBusVoiceLimits;

        std::mutex mDSPMutex;
        std::unordered_map<MIX_Track*, TrackState> mTrackStates;
    };
} // namespace CE::Core::Audio
