#include <atomic>
#include <stdexcept>
#include <SDL3/SDL_audio.h>

#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Core::Audio {
    namespace {
        std::atomic_int gMixInitRefCount{0};
    }

    AudioSystem::AudioSystem(Common::FS::VFS::VFS& vfs, int instanceid, uint32_t device_id, bool stero) : mVFS(vfs) {
        mInstanceID = instanceid;
        if (gMixInitRefCount.fetch_add(1) == 0) {
            if (!MIX_Init()) {
                CE_LOG(LogLevel::Fatal, "[Audio {}] Failed to create audio subsystem!", mInstanceID);
                CE_LOG(LogLevel::Fatal, "[Audio {}] Error from SDL: {}", mInstanceID, SDL_GetError());
                gMixInitRefCount.fetch_sub(1);
                throw std::runtime_error("Failed to create audio subsystem");
            }
        }
        SetAudioDevice(device_id, stero);
    }

    AudioSystem::~AudioSystem() {
        StopAll();

        for (MIX_Track* track : mTracks) {
            {
                std::lock_guard<std::mutex> lock(mDSPMutex);
                mTrackStates.erase(track);
            }
            MIX_DestroyTrack(track);
        }
        mTracks.clear();

        if (mMixer) {
            MIX_DestroyMixer(mMixer);
            mMixer = nullptr;
        }

        if (gMixInitRefCount.fetch_sub(1) == 1) {
            MIX_Quit();
        }
    }

    bool AudioSystem::SetAudioDevice(uint32_t device_id, bool stero) {
        StopAll();

        for (MIX_Track* track : mTracks) {
            {
                std::lock_guard<std::mutex> lock(mDSPMutex);
                mTrackStates.erase(track);
            }
            MIX_DestroyTrack(track);
        }
        mTracks.clear();

        if (mMixer) {
            MIX_DestroyMixer(mMixer);
            mMixer = nullptr;
        }

        SDL_AudioSpec spec = {};
        if (stero) {
            spec.channels = 2;
        } else {
            spec.channels = 1;
        }
        spec.freq = 48000;
        spec.format = SDL_AUDIO_F32;
        mMixer = MIX_CreateMixerDevice(static_cast<SDL_AudioDeviceID>(device_id), &spec);
        if (!mMixer) {
            CE_LOG(LogLevel::Error, "[Audio {}] Failed to create mixer device: {}", mInstanceID, SDL_GetError());
            return false;
        }

        if (!MIX_GetMixerFormat(mMixer, &mMixerSpec)) {
            CE_LOG(LogLevel::Warn, "[Audio {}] Failed to query mixer format: {}", mInstanceID, SDL_GetError());
            mMixerSpec = spec;
        }

        return true;
    }

    std::vector<AudioDeviceInfo> AudioSystem::ListAudioDevices() {
        std::vector<AudioDeviceInfo> devices;
        int count = 0;
        SDL_AudioDeviceID* ids = SDL_GetAudioPlaybackDevices(&count);
        for (int i = 0; i < count; i++) {
            AudioDeviceInfo info;
            info.Id = static_cast<uint32_t>(ids[i]);
            info.Name = SDL_GetAudioDeviceName(ids[i]);
            SDL_AudioSpec spec;
            int sample_frames = 0;
            SDL_GetAudioDeviceFormat(ids[i], &spec, &sample_frames);
            info.stereo = spec.channels >= 2;

            devices.push_back(info);
        }
        SDL_free(ids);
        return devices;
    }

    SDL_AudioDeviceID AudioSystem::GetAudioDeviceID(const std::string& name) {
        if (name == "DefaultAudioDevice") {
            return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        }    
    
        int count = 0;
        SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

        for (int i = 0; i < count; ++i) {
            if (name == SDL_GetAudioDeviceName(devices[i])) {
                SDL_AudioDeviceID id = devices[i];
                SDL_free(devices);
                return id;
            }
        }

        CE_LOG(LogLevel::Error, "[GetAudioDeviceID] Failed to find audio device: '{}'", name);

        SDL_free(devices);
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    bool AudioSystem::IsAudioDeviceStereo(SDL_AudioDeviceID id) {
        SDL_AudioSpec spec;
        int sample_frames = 0;
        SDL_GetAudioDeviceFormat(id, &spec, &sample_frames);
        return spec.channels >= 2;
    }
} // namespace CE::Core::Audio
