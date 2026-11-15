#include "engine/instance/game_instance.hpp"

#include <format>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/fs/os_fs/directory_file_provider.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/common/sdl_events.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/platforms.hpp"
#include "engine/scripting/bindings/as_cubemap.hpp"

namespace CE {
    GameInstance::GameInstance(
        const char* data_file_path,
        bool debugmode,
        Renderer::GPUDeviceHandle& gpudevice,
        EngineArguements args
    ) {
        InitialiseBase(debugmode, gpudevice, std::move(args), data_file_path ? data_file_path : "");
    }

    GameInfo& GameInstance::GetGameInfo() {
        return *mGameInfo;
    }

    Settings::SettingsManager& GameInstance::GetSettingsManager() {
        return *mSettingsManager;
    }

    void GameInstance::PrepareRuntimeData() {
        mGameInfo = std::make_unique<GameInfo>();

        CE_LOG(LogLevel::Info, "[Instance {}] Creating game info", mInstanceID);
        int gis_return = Bootstrap::Init_GameInfo(mVFS, mGameInfo, mDebug);
        if (gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to get gameinfo with code: {}", mInstanceID, gis_return));
        }

        mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>(
            "/config", 100, Platforms::GetConfigPath(GetGameInfo().game_package_id));
        mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>(
            "/saves", 100, Platforms::GetSavePath(GetGameInfo().game_package_id));

        mSettingsManager = std::make_unique<Settings::SettingsManager>(GetGameInfo(), mInstanceID);
        mSettingsManager->SetInstance(*this);
        mSettingsManager->ReloadSettings();
    }

    void GameInstance::HandleWindowEvents() {
        auto indices = SDL_Events::GetWindowEventIndices(mInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = SDL_Events::gEvents[i];

            switch (e.type) { // NOLINT
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                mShouldExit = true;
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                const auto window_mode = mWindow->GetWindowMode();
                if (GetGameInfo().pauseRenderingWhenFocusLostInWindowedMode &&
                    window_mode != Common::Window::WindowMode::Fullscreen &&
                    window_mode != Common::Window::WindowMode::Borderless) {
                    mShouldRender = false;
                }

                if (window_mode == Common::Window::WindowMode::Fullscreen) {
                    mShouldRender = false;
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_FULLSCREEN");
                } else if (window_mode == Common::Window::WindowMode::Borderless) {
                    mShouldRender = false;
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_BORDERLESS");
                } else {
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_WINDOWED");
                }

                if (GetGameInfo().pauseUpdateWhenFocusLost) {
                    mWindowFocus = false;
                }
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                const auto window_mode = mWindow->GetWindowMode();
                if (GetGameInfo().pauseUpdateWhenFocusLost) {
                    mWindowFocus = true;
                }
                mShouldRender = true;
                if (window_mode == Common::Window::WindowMode::Fullscreen) {
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_GAINED_FULLSCREEN");
                } else if (window_mode == Common::Window::WindowMode::Borderless) {
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_GAINED_BORDERLESS");
                } else {
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_GAINED_WINDOWED");
                }
                break;
            }
            }
        }
    }

    void GameInstance::ApplySettingsReload() {
        const int targetW = std::max(1, GetSettingsManager().Settings.windowWidth);
        const int targetH = std::max(1, GetSettingsManager().Settings.windowHeight);

        mWindow->SetWindowSize({targetW, targetH}, GetSettingsManager().Settings.windowMode);

        if (GetSettingsManager().Settings.windowMode != mWindow->GetWindowMode()) {
            mWindow->SetWindowMode(GetSettingsManager().Settings.windowMode);
        }

        mRenderer->SetVSync(GetSettingsManager().Settings.enableVSync);

        if (mAudioManager) {
            mAudioManager->SetMasterVolume(GetSettingsManager().Settings.masterVolume);
            mAudioManager->SetMusicVolume(GetSettingsManager().Settings.musicVolume);
            mAudioManager->SetSFXVolume(GetSettingsManager().Settings.sfxVolume);
        }

        mGameStateManager->Emit("CE_SETTINGS_RELOAD");
    }

    void GameInstance::OnInitialise() {
        mScriptingManager = std::make_unique<CE::Scripting::Runtime>(
            *mVFS,
            *mGameInfo,
            *mSettingsManager,
            *this,
            *mRenderer,
            *mModelRenderer,
            *mTextureManager,
            *mShaderManager,
            *mFontManager,
            *m3DModelImporter,
            *mGPUMeshManager,
            *mMaterialManager,
            *mAnimatedTextureManager,
            *mKeyboardManger,
            *mMouseManger,
            *mTextInputManager,
            *mInputBindingManager,
            *mRendererResourcesNameRegistry,
            mProgramArguments.OutputDebugASInfo,
            mProgramArguments.OutputDebugASInfoPath,
            *mWindow,
            *mGameStateManager,
            mAudioManager.get(),
            mAudioSystem.get()
        );

        if (!mScriptingManager->Init()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format(
                "[Instance {}] AngelScript initialization failed: {}",
                GetInstanceID(),
                mScriptingManager->GetLastError()));
        }

        if (!mScriptingManager->RunStartup()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format(
                "[Instance {}] AngelScript startup failed: {}",
                GetInstanceID(),
                mScriptingManager->GetLastError()));
        }

        mASDebuggerWindow = std::make_unique<UI::AngelscriptDebuggerWindow>(
            mScriptingManager->GetScriptDebugger(), *mScriptingManager, *mVFS);

        mDebugWindow = std::make_unique<UI::DebugWindow>(
            *mRenderer,
            *mTextureManager,
            *mShaderManager,
            *mFontManager,
            *mGameInfo,
            *mSettingsManager,
            mAudioManager.get(),
            *mKeyboardManger,
            *this,
            *mMouseManger,
            mAudioSystem.get(),
            mASDebuggerWindow.get(),
            mLogsWindow);
    }

    bool GameInstance::OnLogic() {
        CE::Scripting::Bindings::ASCubemap::SetDefaultTextureManager(*mTextureManager);
        return true;
    }

    bool GameInstance::OnDraw2D() {
        if (!mScriptingManager->RunUpdate()) {
            ShowError(mScriptingManager->GetLastError());
            CE_LOG(LogLevel::Error,
                   "[Instance {}] AngelScript update failed, shutting down instance",
                   GetInstanceID());
            RequestExit();
            return false;
        }
        return true;
    }

    bool GameInstance::OnImGui() {
        if (mDebugWindow) {
            mDebugWindow->Draw();
        }

        if (mASDebuggerWindow) {
            mASDebuggerWindow->DrawWindow();
        }

        mLogsWindow.DrawWindow();

        if (!mScriptingManager->RunImGui()) {
            ShowError(mScriptingManager->GetLastError());
            CE_LOG(LogLevel::Error,
                   "[Instance {}] AngelScript ImGui update failed, shutting down instance",
                   GetInstanceID());
            RequestExit();
            return false;
        }

        return true;
    }

    GameInstance::~GameInstance() {
        mASDebuggerWindow.reset();
        mDebugWindow.reset();
        mScriptingManager.reset();
    }
} // namespace CE
