#include "engine/scripting/angelscript.hpp"

#include <scriptstdstring/scriptstdstring.h>

namespace CE::Scripting {
    Runtime::Runtime(
        VFS::VFS& vfs,
        CE::GameInfo& gameInfo,
        Settings::SettingsManager& settingsManager,
        Instance& instance,
        Renderer::IRenderer& renderer,
        Assets::Textures::TextureManager& textureManager,
        Assets::Fonts::FontManager& fontManager,
        Input::Keyboard& keyboard,
        Input::Mouse& mouse
    )
        : mVFS(vfs)
        , mGameInfo(gameInfo)
        , mSettingsManager(settingsManager)
        , mInstance(instance)
        , mRenderer(renderer)
        , mTextureManager(textureManager)
        , mFontManager(fontManager)
        , mKeyboard(keyboard)
        , mMouse(mouse) {
    }

    Runtime::~Runtime() {
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
        mScriptEngine = asCreateScriptEngine();
        if (mScriptEngine == nullptr) {
            return false;
        }

        RegisterStdString(mScriptEngine);

        if (!RegisterAssetsBindings()) {
            return false;
        }

        if (!RegisterInputBindings()) {
            return false;
        }

        if (!RegisterInstanceBindings()) {
            return false;
        }

        mContext = mScriptEngine->CreateContext();
        return mContext != nullptr;
    }

    void Runtime::RunStartup() {
        mScriptModule = mScriptEngine->GetModule("Main", asGM_ALWAYS_CREATE);
        
    }
}
