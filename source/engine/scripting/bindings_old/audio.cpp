#include "engine/assets/audio.hpp"

#include "engine/common/tracelog.hpp"
#include "engine/scripting/angelscript.hpp"

namespace CE::Scripting {
    namespace {
        constexpr int kAudioTypeSFX = 0;
        constexpr int kAudioTypeMusic = 1;

        constexpr int kEffectTypeLowPass = 0;
        constexpr int kEffectTypeHighPass = 1;
        constexpr int kEffectTypeReverb = 2;
        constexpr int kEffectTypeDelay = 3;
        constexpr int kEffectTypeChorus = 4;
    } // namespace

    bool Runtime::RegisterAudioBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        int result = 0;
        mScriptEngine->SetDefaultNamespace("CE::Audio");

        result = mScriptEngine->RegisterEnum("AudioType");
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("AudioType", "SFX", kAudioTypeSFX);
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("AudioType", "Music", kAudioTypeMusic);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnum("EffectType");
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("EffectType", "LowPass", kEffectTypeLowPass);
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("EffectType", "HighPass", kEffectTypeHighPass);
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("EffectType", "Reverb", kEffectTypeReverb);
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("EffectType", "Delay", kEffectTypeDelay);
        if (result < 0) {
            return false;
        }
        result = mScriptEngine->RegisterEnumValue("EffectType", "Chorus", kEffectTypeChorus);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectType("AudioEffect", sizeof(AudioEffectDesc),
                                                   asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<AudioEffectDesc>());
        if (result < 0) {
            return false;
        }

        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "bool enabled", asOFFSET(AudioEffectDesc, enabled));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "EffectType type", asOFFSET(AudioEffectDesc, type));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float cutoffHz", asOFFSET(AudioEffectDesc, cutoffHz));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float wetMix", asOFFSET(AudioEffectDesc, wetMix));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float feedback", asOFFSET(AudioEffectDesc, feedback));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float delayMs", asOFFSET(AudioEffectDesc, delayMs));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float depthMs", asOFFSET(AudioEffectDesc, depthMs));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float rateHz", asOFFSET(AudioEffectDesc, rateHz));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float roomSize", asOFFSET(AudioEffectDesc, roomSize));
        if (result < 0) {
            return false;
        }
        result =
            mScriptEngine->RegisterObjectProperty("AudioEffect", "float damping", asOFFSET(AudioEffectDesc, damping));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void LoadSound(const string &in path, const string &in name, AudioType type)",
            asMETHOD(Runtime, LoadSound), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void UnloadSound(const string &in name)",
                                                       asMETHOD(Runtime, UnloadSound), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("uint CreateInstance(const string &in name)",
                                                       asMETHOD(Runtime, CreateSoundInstance), asCALL_THISCALL_ASGLOBAL,
                                                       this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DeleteInstance(uint handle)", asMETHOD(Runtime, DeleteSoundInstance), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void Play(uint handle)", asMETHOD(Runtime, PlaySound),
                                                       asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void Pause(uint handle)", asMETHOD(Runtime, PauseSound),
                                                       asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void Resume(uint handle)", asMETHOD(Runtime, ResumeSound),
                                                       asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void Stop(uint handle)", asMETHOD(Runtime, StopSound),
                                                       asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void Seek(uint handle, float seconds)",
                                                       asMETHOD(Runtime, SeekSound), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void SetBus(uint handle, const string &in bus)",
                                                       asMETHOD(Runtime, SetSoundBus), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("string GetBus(uint handle)", asMETHOD(Runtime, GetSoundBus),
                                                       asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result =
            mScriptEngine->RegisterGlobalFunction("void SetVolume(uint handle, int volume)",
                                                  asMETHOD(Runtime, SetSoundVolume), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetMasterVolume(float volume)", asMETHOD(Runtime, SetMasterVolume), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetMusicVolume(float volume)", asMETHOD(Runtime, SetMusicVolume), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void SetSFXVolume(float volume)",
                                                       asMETHOD(Runtime, SetSFXVolume), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void AddEffect(uint handle, const string &in name, const AudioEffect &in effect)",
            asMETHOD(Runtime, AddEffect), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void RemoveEffect(uint handle, const string &in name)",
                                                       asMETHOD(Runtime, RemoveEffect), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction("void ClearEffects(uint handle)",
                                                       asMETHOD(Runtime, ClearEffects), asCALL_THISCALL_ASGLOBAL, this);
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    void Runtime::LoadSound(const std::string& path, const std::string& name, int type) {
        if (!mAudioManager) {
            return;
        }

        Core::Audio::AudioType resolved = Core::Audio::AudioType::SFX;
        if (type == kAudioTypeMusic) {
            resolved = Core::Audio::AudioType::Music;
        }
        mAudioManager->LoadSound(path, name, resolved);
    }

    void Runtime::UnloadSound(const std::string& name) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->UnloadSound(name);
    }

    uint32_t Runtime::CreateSoundInstance(const std::string& name) {
        if (!mAudioManager) {
            return 0;
        }
        return mAudioManager->CreateSoundInstance(name);
    }

    void Runtime::DeleteSoundInstance(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->DeleteSoundInstance(handle);
    }

    void Runtime::PlaySound(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->PlaySound(handle);
    }

    void Runtime::PauseSound(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->PauseSound(handle);
    }

    void Runtime::ResumeSound(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->ResumeSound(handle);
    }

    void Runtime::StopSound(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->StopSound(handle);
    }

    void Runtime::SeekSound(uint32_t handle, float seconds) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SeekSound(handle, seconds);
    }

    void Runtime::SetSoundBus(uint32_t handle, const std::string& bus) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SetSoundBus(handle, bus);
    }

    std::string Runtime::GetSoundBus(uint32_t handle) {
        if (!mAudioManager) {
            return {};
        }
        return mAudioManager->GetSoundBus(handle);
    }

    void Runtime::SetSoundVolume(uint32_t handle, int volume) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SetSoundVolume(handle, volume);
    }

    void Runtime::SetMasterVolume(float volume) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SetMasterVolume(volume);
    }

    void Runtime::SetMusicVolume(float volume) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SetMusicVolume(volume);
    }

    void Runtime::SetSFXVolume(float volume) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->SetSFXVolume(volume);
    }

    void Runtime::AddEffect(uint32_t handle, const std::string& name, const AudioEffectDesc& effect) {
        if (!mAudioManager) {
            return;
        }

        Core::Audio::AudioFilter filter;
        filter.Enabled = effect.enabled;
        filter.Kind = static_cast<Core::Audio::AudioFilter::Type>(effect.type);
        filter.CutoffHz = effect.cutoffHz;
        filter.WetMix = effect.wetMix;
        filter.Feedback = effect.feedback;
        filter.DelayMs = effect.delayMs;
        filter.DepthMs = effect.depthMs;
        filter.RateHz = effect.rateHz;
        filter.RoomSize = effect.roomSize;
        filter.Damping = effect.damping;

        mAudioManager->AddEffect(handle, name, filter);
    }

    void Runtime::RemoveEffect(uint32_t handle, const std::string& name) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->RemoveEffect(handle, name);
    }

    void Runtime::ClearEffects(uint32_t handle) {
        if (!mAudioManager) {
            return;
        }
        mAudioManager->ClearEffects(handle);
    }
} // namespace CE::Scripting
