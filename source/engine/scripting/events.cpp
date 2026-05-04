#include "engine/scripting/angelscript.hpp"

#include "engine/common/tracelog.hpp"

#include <format>

namespace CE::Scripting {
    bool Runtime::InvokeStateCallback(
        asIScriptFunction* callback,
        const std::string& state,
        const std::string& eventName
    ) {
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
            return Fail(std::format(
                "Failed to prepare callback '{}' with code {}",
                callback->GetDeclaration(),
                result
            ));
        }

        ctx->SetArgObject(0, const_cast<std::string*>(&state));
        ctx->SetArgObject(1, const_cast<std::string*>(&eventName));

        result = ctx->Execute();
        if (result != asEXECUTION_FINISHED) {
            const char* exception = ctx->GetExceptionString();
            const std::string message = std::format(
                "Callback '{}' failed with code {}{}{}",
                callback->GetDeclaration(),
                result,
                exception ? ": " : "",
                exception ? exception : ""
            );
            ctx->Release();
            return Fail(message);
        }

        ctx->Release();
        return true;
    }

    void Runtime::ReleaseStateCallbacks() {
        for (auto& registration : mStateCallbacks) {
            if (registration.function != nullptr) {
                registration.function->Release();
                registration.function = nullptr;
            }
        }

        mStateCallbacks.clear();
    }
}
