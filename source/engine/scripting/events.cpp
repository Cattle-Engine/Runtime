#include <format>
#include <algorithm>

#include "engine/scripting/angelscript.hpp"
#include "engine/common/core/game_state.hpp"

namespace CE::Scripting {
    void Runtime::UnsubscribeStateEvent(int id) {
        auto it = std::find_if(
            mStateCallbacks.begin(),
            mStateCallbacks.end(),
            [id](const ScriptCallbackRegistration& registration) {
                return registration.id == id;
            });

        if (it == mStateCallbacks.end()) {
            return;
        }

        mGameStateManager.Unsubscribe(
            it->state,
            it->eventName,
            it->id);

        if (it->function != nullptr) {
            it->function->Release();
        }

        mStateCallbacks.erase(it);
    }

    int Runtime::SubscribeStateEvent(
        const std::string& state,
        const std::string& eventName,
        asIScriptFunction* function
    ) {

        if (function == nullptr) {
            return -1;
        }

        function->AddRef();

        const int id = mGameStateManager.Subscribe(
            state,
            eventName,
            [this, function](std::string_view emittedState,
                            std::string_view emittedEvent) {
                InvokeStateCallback(
                    function,
                    std::string(emittedState),
                    std::string(emittedEvent));
            });

        if (id < 0) {
            function->Release();
            return -1;
        }

        mStateCallbacks.push_back({
            state,
            eventName,
            id,
            function
        });

        return id;
    }

    bool Runtime::InvokeStateCallback(asIScriptFunction* callback, const std::string& state,
                                      const std::string& eventName) {
        if (callback == nullptr || mScriptEngine == nullptr) {
            return false;
        }

        asIScriptContext* ctx = mScriptEngine->CreateContext();
        if (ctx == nullptr) {
            return Fail("Failed to create AngelScript callback context");
        }

        int result = ctx->Prepare(callback);
        if (result < 0) {
            ctx->Release();
            return Fail(
                std::format("Failed to prepare callback '{}' with code {}", callback->GetDeclaration(), result));
        }

        ctx->SetArgObject(0, const_cast<std::string*>(&state));
        ctx->SetArgObject(1, const_cast<std::string*>(&eventName));

        result = ctx->Execute();
        if (result != asEXECUTION_FINISHED) {
            const char* exception = ctx->GetExceptionString();
            const std::string message = std::format("Callback '{}' failed with code {}{}{}", callback->GetDeclaration(),
                                                    result, exception ? ": " : "", exception ? exception : "");
            ctx->Release();
            return Fail(message);
        }

        ctx->Release();
        return true;
    }

    void Runtime::ReleaseStateCallbacks() {
        for (auto& registration : mStateCallbacks) {
            if (registration.id >= 0) {
                mGameStateManager.Unsubscribe(
                    registration.state,
                    registration.eventName,
                    registration.id);

                registration.id = -1;
            }

            if (registration.function != nullptr) {
                registration.function->Release();
                registration.function = nullptr;
            }
        }

        mStateCallbacks.clear();
    }
} // namespace CE::Scripting
