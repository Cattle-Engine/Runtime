#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Core::Audio {
    AudioSystem::AudioSystem(VFS::VFS& vfs, int instanceid, uint32_t device_id, bool stero) : mVFS(vfs) {
        mInstanceID = instanceid;
        if(!MIX_Init()) {
            CE::Log(LogLevel::Fatal, "[Audio {}] Failed to create audio subsystem!", mInstanceID);
            CE::Log(LogLevel::Fatal, "[Audio {}] Error from SDL: ", mInstanceID, SDL_GetError());
            throw std::runtime_error("Failed to create audio subsystem");
        }
        SetAudioDevice(device_id, stero);
    }

    void AudioSystem::SetAudioDevice(uint32_t device_id, bool stero) {
        SDL_AudioSpec spec = {};
        if (stero) {
            spec.channels = 2;
        } else {
            spec.channels = 1;
        }
        spec.freq = 48000;
        spec.format = SDL_AUDIO_F32;
        mMixer = MIX_CreateMixerDevice(static_cast<SDL_AudioDeviceID>(device_id), &spec);
    }

    std::vector<AudioDeviceInfo> AudioSystem::ListAudioDevices() {
        std::vector<AudioDeviceInfo> devices;
        int count = 0;
        SDL_AudioDeviceID* ids = SDL_GetAudioPlaybackDevices(&count);
        for (int i = 0; i < count; i++) {
            AudioDeviceInfo info;
            info.Id = static_cast<uint32_t>(ids[i]);
            info.Name = SDL_GetAudioDeviceName(ids[i]);
            devices.push_back(info);
        }
        return devices;
    }
}