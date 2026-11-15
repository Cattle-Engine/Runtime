#include <memory>
#include <SDL3/SDL.h>

#include "engine/instance/base_instance.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/settings.hpp"

namespace CE {
    int BaseInstance::Bootstrap_Audio() {
        try {
            CE_LOG(LogLevel::Info, "[Instance {}] SDL audio drivers:", mInstanceID);
            for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
                CE_LOG(LogLevel::Info, "  {}", SDL_GetAudioDriver(i));
            }

            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                CE_LOG(LogLevel::Error, "[Instance {}] Failed to init SDL audio: {}", mInstanceID, SDL_GetError());
                throw std::runtime_error("Failed to init SDL audio");
            }
            mAudioSubsystemInited = true;

            CE_LOG(LogLevel::Info, "[Instance {}] SDL audio driver: {}", mInstanceID, SDL_GetCurrentAudioDriver());
            CE_LOG(LogLevel::Info, "[Instance {}] Creating audio system", mInstanceID);

            SDL_AudioDeviceID audio_device_id =
                Core::Audio::AudioSystem::GetAudioDeviceID(mSettingsManager->Settings.audioDeviceName);
            mAudioSystem = std::make_unique<Core::Audio::AudioSystem>(
                *mVFS,
                mInstanceID,
                static_cast<uint32_t>(audio_device_id),
                Core::Audio::AudioSystem::IsAudioDeviceStereo(audio_device_id));

            mAudioManager = std::make_unique<Audio::Resources::AudioManager>(*mAudioSystem, *mVFS, mInstanceID);
            mAudioManager->SetMasterVolume(mSettingsManager->Settings.masterVolume);
            mAudioManager->SetMusicVolume(mSettingsManager->Settings.musicVolume);
            mAudioManager->SetSFXVolume(mSettingsManager->Settings.sfxVolume);
        } catch (const std::exception& e) {
            CE_LOG(LogLevel::Error, "[Instance {}] Audio system failed to initialize: {}", mInstanceID, e.what());
            mAudioManager.reset();
            mAudioSystem.reset();
        }
        return 0; // audio is optional
    }
}