#include "engine/scripting/angelscript.hpp"

#include "engine/common/tracelog.hpp"

#include <format>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>

namespace {
    const char* ToString(asEMsgType type) {
        switch (type) {
            case asMSGTYPE_ERROR:
                return "Error";
            case asMSGTYPE_WARNING:
                return "Warning";
            case asMSGTYPE_INFORMATION:
                return "Info";
            default:
                return "Unknown";
        }
    }
}

namespace CE::Scripting {
    Runtime::Runtime(
        VFS::VFS& vfs,
        CE::GameInfo& gameInfo,
        Settings::SettingsManager& settingsManager,
        Instance& instance,
        Renderer::IRenderer& renderer,
        Assets::Textures::TextureManager& textureManager,
        Assets::Shaders::ShaderManager& shaderManager,
        Assets::Fonts::FontManager& fontManager,
        CE::Assets::Animations::AnimationManager* animationManager,
        Input::Keyboard& keyboard,
        Input::Mouse& mouse,
        CE::Assets::Audio::AudioManager* audioManager
    )
        : mVFS(vfs)
        , mGameInfo(gameInfo)
        , mSettingsManager(settingsManager)
        , mInstance(instance)
        , mRenderer(renderer)
        , mTextureManager(textureManager)
        , mShaderManager(shaderManager)
        , mFontManager(fontManager)
        , mAnimationManager(animationManager)
        , mKeyboard(keyboard)
        , mMouse(mouse)
        , mAudioManager(audioManager) {
    }

    Runtime::~Runtime() {
        ReleaseStateCallbacks();

        if (mUpdateCtx != nullptr) {
            mUpdateCtx->Release();
            mUpdateCtx = nullptr;
        }

        if (mContext != nullptr) {
            mContext->Release();
            mContext = nullptr;
        }

        if (mScriptEngine != nullptr) {
            mScriptEngine->ShutDownAndRelease();
            mScriptEngine = nullptr;
        }
    }

    bool Runtime::Initialize() {
        mLastError.clear();
        mScriptEngine = asCreateScriptEngine();
        if (mScriptEngine == nullptr) {
            return Fail("Failed to create AngelScript engine");
        }

        CE::Log(CE::LogLevel::Info, "[AngelScript] Engine created");
        mScriptEngine->SetMessageCallback(asFUNCTION(MessageCallback), this, asCALL_CDECL_OBJLAST);
        RegisterStdString(mScriptEngine);
        RegisterScriptArray(mScriptEngine, true);

        if (!RegisterAssetsBindings()) {
            return Fail("Failed to register AngelScript asset bindings");
        }

        if (!RegisterInputBindings()) {
            return Fail("Failed to register AngelScript input bindings");
        }

        if (!RegisterInstanceBindings()) {
            return Fail("Failed to register AngelScript instance bindings");
        }

        if (!RegisterCallbackBindings()) {
            return Fail("Failed to register AngelScript callback bindings");
        }

        if (!RegisterAudioBindings()) {
            return Fail("Failed to register AngelScript audio bindings");
        }

        mContext = mScriptEngine->CreateContext();
        if (mContext == nullptr) {
            return Fail("Failed to create AngelScript context");
        }

        CE::Log(CE::LogLevel::Info, "[AngelScript] Runtime initialized");
        return true;
    }

    bool Runtime::RunStartup() {
        mLastError.clear();
        mScriptModule = mScriptEngine->GetModule("Startup", asGM_ALWAYS_CREATE);
        if (mScriptModule == nullptr) {
            return Fail("Failed to create AngelScript module");
        }

        std::string code =
            Utils::LoadScript(mVFS, mGameInfo.startupFileName.c_str());

        if (code.empty()) {
            return Fail(std::format("Failed to load AngelScript startup file '{}'", mGameInfo.startupFileName));
        }

        CE::Log(CE::LogLevel::Info, "[AngelScript] Loaded startup script '{}'", mGameInfo.startupFileName);

        int r = mScriptModule->AddScriptSection("startup", code.c_str());
        if (r < 0) {
            return Fail("Failed to add AngelScript startup script section");
        }

        r = mScriptModule->Build();
        if (r < 0) {
            return Fail("Failed to build AngelScript module");
        }

        asIScriptFunction* func =
            mScriptModule->GetFunctionByDecl("void main()");

        if (!func) {
            return Fail("AngelScript entrypoint 'void main()' was not found");
        }

        asIScriptContext* ctx = mScriptEngine->CreateContext();
        if (ctx == nullptr) {
            return Fail("Failed to create AngelScript startup context");
        }

        ctx->Prepare(func);

        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED) {
            ctx->Release();
            return Fail(std::format("AngelScript main() execution failed with code {}", r));
        }

        ctx->Release();
        mUpdateFunc = mScriptModule->GetFunctionByDecl("void update()");
        if (mUpdateFunc == nullptr) {
            CE::Log(CE::LogLevel::Warn, "[AngelScript] No 'void update()' function found");
            return true;
        }

        mUpdateCtx = mScriptEngine->CreateContext();
        if (mUpdateCtx == nullptr) {
            return Fail("Failed to create AngelScript update context");
        }

        CE::Log(CE::LogLevel::Info, "[AngelScript] Startup completed");
        return true;
    }

    bool Runtime::RunUpdate() {
        if (!mUpdateFunc || !mUpdateCtx)
            return true;

        mLastError.clear();
        int r = mUpdateCtx->Prepare(mUpdateFunc);
        if (r < 0) {
            return Fail(std::format("Failed to prepare AngelScript update() with code {}", r));
        }

        r = mUpdateCtx->Execute();
        if (r != asEXECUTION_FINISHED) {
            return Fail(std::format("AngelScript update() execution failed with code {}", r));
        }

        return true;
    }

    const std::string& Runtime::GetLastError() const {
        return mLastError;
    }

    void Runtime::MessageCallback(const asSMessageInfo* msg, void* param) {
        auto* runtime = static_cast<Runtime*>(param);
        if (msg == nullptr || runtime == nullptr) {
            return;
        }

        const std::string message = std::format(
            "[AngelScript] {}:{}:{} {}: {}",
            msg->section ? msg->section : "<unknown>",
            msg->row,
            msg->col,
            ToString(msg->type),
            msg->message ? msg->message : ""
        );

        switch (msg->type) {
            case asMSGTYPE_ERROR:
                runtime->mLastError = message;
                CE::Log(CE::LogLevel::Error, "{}", message);
                break;
            case asMSGTYPE_WARNING:
                CE::Log(CE::LogLevel::Warn, "{}", message);
                break;
            case asMSGTYPE_INFORMATION:
            default:
                CE::Log(CE::LogLevel::Info, "{}", message);
                break;
        }
    }

    bool Runtime::Fail(const std::string& message) {
        if (mLastError.empty()) {
            mLastError = message;
        }

        CE::Log(CE::LogLevel::Error, "{}", mLastError);
        return false;
    }
}
