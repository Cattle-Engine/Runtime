#include "engine/scripting/angelscript.hpp"
#include "engine/instance.hpp"

namespace CE::Scripting {
    bool Runtime::RegisterInstanceBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        int result = 0;

        mScriptEngine->SetDefaultNamespace("CE");
        result = mScriptEngine->RegisterGlobalFunction(
            "void Exit()",
            asMETHOD(Runtime, ExitInstance),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "float GetDeltaTime()",
            asMETHOD(Runtime, GetDeltaTime),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "float GetFrameTime()",
            asMETHOD(Runtime, GetFrameTime),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "int GetFPS()",
            asMETHOD(Runtime, GetFPS),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "int GetInstanceID()",
            asMETHOD(Runtime, GetInstanceID),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Settings");

        result = mScriptEngine->RegisterGlobalFunction(
            "void ReloadSettings()",
            asMETHOD(Runtime, ReloadSettings),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "int GetSettingInt(const string &in key, const string &in section, int fallback)",
            asMETHOD(Runtime, GetSettingInt),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "float GetSettingFloat(const string &in key, const string &in section, float fallback)",
            asMETHOD(Runtime, GetSettingFloat),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool GetSettingBool(const string &in key, const string &in section, bool fallback)",
            asMETHOD(Runtime, GetSettingBool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "string GetSettingString(const string &in key, const string &in section, const string &in fallback)",
            asMETHOD(Runtime, GetSettingString),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSettingInt(const string &in key, const string &in section, int value)",
            asMETHOD(Runtime, SetSettingInt),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSettingFloat(const string &in key, const string &in section, float value)",
            asMETHOD(Runtime, SetSettingFloat),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSettingBool(const string &in key, const string &in section, bool value)",
            asMETHOD(Runtime, SetSettingBool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSettingString(const string &in key, const string &in section, const string &in value)",
            asMETHOD(Runtime, SetSettingString),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::State");

        result = mScriptEngine->RegisterGlobalFunction(
            "void Set(const string &in state)",
            asMETHOD(Runtime, SetGameState),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "string Get()",
            asMETHOD(Runtime, GetGameState),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        mScriptEngine->SetDefaultNamespace("");
        return result >= 0;
    }

    bool Runtime::RegisterCallbackBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        int result = mScriptEngine->RegisterFuncdef(
            "void StateCallback(const string &in state, const string &in eventName)"
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Events");
        result = mScriptEngine->RegisterGlobalFunction(
            "int On(const string &in state, const string &in eventName, StateCallback @callback)",
            asMETHOD(Runtime, RegisterStateCallback),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        mScriptEngine->SetDefaultNamespace("");
        return result >= 0;
    }

    void Runtime::ExitInstance() {
        mInstance.Exit();
    }

    float Runtime::GetDeltaTime() {
        return mInstance.GetDeltaTime();
    }

    float Runtime::GetFrameTime() {
        return mInstance.GetFrameTime();
    }

    int Runtime::GetFPS() {
        return mInstance.GetFPS();
    }

    int Runtime::GetInstanceID() {
        return mInstance.GetInstanceID();
    }

    void Runtime::ReloadSettings() {
        mSettingsManager.ReloadSettings();
    }

    int Runtime::RegisterStateCallback(
        const std::string& state,
        const std::string& eventName,
        asIScriptFunction* callback
    ) {
        if (callback == nullptr) {
            Fail("CE::Events::On received a null callback");
            return -1;
        }

        callback->AddRef();

        const int id = mInstance.GetEventBus().Subscribe(
            state,
            eventName,
            [this, callback](std::string_view emittedState, std::string_view emittedEventName) {
                if (!InvokeStateCallback(
                        callback,
                        std::string(emittedState),
                        std::string(emittedEventName))) {
                    mInstance.Exit();
                }
            }
        );

        mStateCallbacks.push_back({
            state,
            eventName,
            id,
            callback
        });

        return id;
    }

    void Runtime::SetGameState(const std::string& state) {
        mInstance.SetGameState(state);
    }

    std::string Runtime::GetGameState() const {
        return mInstance.GetGameState();
    }

    int Runtime::GetSettingInt(const std::string& key, const std::string& section, int fallback) {
        return mSettingsManager.Custom_GetInteger(key, section, fallback);
    }

    float Runtime::GetSettingFloat(const std::string& key, const std::string& section, float fallback) {
        return mSettingsManager.Custom_GetFloat(key, section, fallback);
    }

    bool Runtime::GetSettingBool(const std::string& key, const std::string& section, bool fallback) {
        return mSettingsManager.Custom_GetBool(key, section, fallback);
    }

    std::string Runtime::GetSettingString(const std::string& key, const std::string& section, const std::string& fallback) {
        return mSettingsManager.Custom_GetString(key, section, fallback);
    }

    void Runtime::SetSettingInt(const std::string& key, const std::string& section, int value) {
        mSettingsManager.Custom_SetInteger(key, section, value);
    }

    void Runtime::SetSettingFloat(const std::string& key, const std::string& section, float value) {
        mSettingsManager.Custom_SetFloat(key, section, value);
    }

    void Runtime::SetSettingBool(const std::string& key, const std::string& section, bool value) {
        mSettingsManager.Custom_SetBool(key, section, value);
    }

    void Runtime::SetSettingString(const std::string& key, const std::string& section, const std::string& value) {
        mSettingsManager.Custom_SetString(key, section, value);
    }
}
