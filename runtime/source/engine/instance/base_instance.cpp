#include "engine/instance/base_instance.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/fs/os_fs/directory_file_provider.hpp"
#include "engine/common/sdl_events.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/window.hpp"
#include "engine/platforms.hpp"
#include "engine/settings.hpp"

namespace CE {
    // NOTE: this used to be visible through instance.hpp. Define it in exactly one .cpp
    // (here is fine) and make sure nothing else defines it.
    extern int gGlobalInstanceCounter;

    BaseInstance::~BaseInstance() {
        if (mInstanceID != 0) {
            gGlobalInstanceCounter--;
        }

        // Anything that holds GPU/audio resources must go before the renderer shuts down.
        // Derived destructors have already run, so scripting etc. is already gone.
        mACCS.reset();
        mAudioManager.reset();
        mAudioSystem.reset();
        m3DModelImporter.reset();
        mFontManager.reset();
        mAnimatedTextureManager.reset();
        mShaderManager.reset();
        mModelRenderer.reset();
        mGPUMeshManager.reset();
        mMaterialManager.reset();
        mTextureManager.reset();

        if (mRenderer && mWindow) {
            mRenderer->Shutdown(mWindow->GetWindow());
        }

        if (mAudioSubsystemInited) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    }

    int BaseInstance::MountData(const std::string& path) {
        return Bootstrap::Init_GameData(mVFS, path.c_str(), mDebug);
    }

    void BaseInstance::InitialiseBase(
        bool debug_mode,
        Renderer::GPUDeviceHandle& gpu_device,
        EngineArguements args,
        const std::string& data_path
    ) {
        if (mWasBaseInitDone) {
            throw std::logic_error("You cannot init twice!");
        }

        mProgramArguments = std::move(args);
        gGlobalInstanceCounter++;
        mInstanceID = gGlobalInstanceCounter;
        mDebug = debug_mode;
        mPerformanceFrequency = SDL_GetPerformanceFrequency();
        mLastFrameCounter = SDL_GetPerformanceCounter();

        mGameStateManager = std::make_unique<Core::GameState::GameStateManager>(mEventBus);
        mVFS = std::make_unique<Common::FS::VFS::VFS>();
        mGameInfo = std::make_unique<GameInfo>();

        CE_LOG(LogLevel::Info, "[Instance {}] Setting up game data", mInstanceID);
        int gds_return = MountData(data_path);
        if (gds_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Gamedata mount returned with code {}", mInstanceID, gds_return));
        }

        CE_LOG(LogLevel::Info, "[Instance {}] Creating game info", mInstanceID);
        int gis_return = Bootstrap::Init_GameInfo(mVFS, mGameInfo, mDebug);
        if (gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to get gameinfo with code: {}", mInstanceID, gis_return));
        }

        mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>(
            "/config", 100, Platforms::GetConfigPath(mGameInfo->game_package_id));
        mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>(
            "/saves", 100, Platforms::GetSavePath(mGameInfo->game_package_id));

        mSettingsManager = std::make_unique<Settings::SettingsManager>(*mGameInfo, mInstanceID);
        mSettingsManager->SetInstance(*this);
        mSettingsManager->ReloadSettings();

        CE_LOG(LogLevel::Info, "[Instance {}] Creating window & renderer", mInstanceID);
        int vis = Bootstrap_Video(gpu_device);
        if (vis != 0) {
            throw std::runtime_error(std::format("[Instance {}] Video setup returned with: {}", mInstanceID, vis));
        }

        mRenderer->SetVSync(mSettingsManager->Settings.enableVSync);
        mInstanceWindowID = SDL_GetWindowID(mWindow->GetWindow());
        if (!mGameInfo->windowIcon.empty()) {
            mWindow->SetWindowIcon(mGameInfo->windowIcon);
        }

        CE_LOG(LogLevel::Info, "[Instance {}] Creating renderer resource managers", mInstanceID);
        int brrms = Bootstrap_RendererResourceManagers();
        if (brrms != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init renderer resource managers: {}", mInstanceID, brrms));
        }

        int aiam = Bootstrap_AssetImportersAndManagers();
        if (aiam != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init asset importers and managers {}", mInstanceID, aiam));
        }

        if (Bootstrap_InputManagers() != 0) {
            throw std::runtime_error(std::format("[Instance {}] Failed to init input managers", mInstanceID));
        }

        Bootstrap_Audio(); // non-fatal, leaves mAudioSystem/mAudioManager null on failure

        CE_LOG(LogLevel::Info, "[Instance {}] Creating ACCS", mInstanceID);
        if (Bootstrap_ACCS() != 0) {
            throw std::runtime_error(std::format("[Instance {}] Failed to init ACCS", mInstanceID));
        }

        mWindowFocus = true;
        mWasBaseInitDone = true;

        OnInitialise();
    }

    int BaseInstance::Update() {
        if (mShouldExit) {
            return 1;
        }

        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        mKeyboardManger->Update();
        mMouseManger->Update();
        mTextInputManager->Update();

        HandleWindowEvents();

        if (mPendingSettingsReload) {
            ApplySettingsReload();
            mPendingSettingsReload = false;
        }

        if (!mWindowFocus) {
            mLastFrameCounter = SDL_GetPerformanceCounter();
            mDeltaTime = 0.0f;
            return 0;
        }

        mRenderer->SetClearColour(255, 255, 255, 255);
        mACCS->system_manager.Update(mDeltaTime, ACCS::FrameUpdateType::Logic, mACCS->world);
        mGameStateManager->Emit("Update");

        if (!OnLogic()) {
            CE_LOG(LogLevel::Error, "[Instance {}] OnLogic failed, shutting down instance", mInstanceID);
            mShouldExit = true;
            return 1;
        }

        if (mShouldRender) {
            if (mRenderer->BeginFrame(mWindow->GetWindow()) != 0) {
                return 1;
            }

            mRenderer->BeginMode3D();
            mACCS->system_manager.Update(mDeltaTime, ACCS::FrameUpdateType::Render3D, mACCS->world);
            mGameStateManager->Emit("Draw3D");
            if (!OnDraw3D()) {
                CE_LOG(LogLevel::Error, "[Instance {}] OnDraw3D failed, shutting down instance", mInstanceID);
                mShouldExit = true;
                return 1;
            }
            mRenderer->EndMode3D();

            mRenderer->BeginMode2D();
            mACCS->system_manager.Update(mDeltaTime, ACCS::FrameUpdateType::Render2D, mACCS->world);
            mGameStateManager->Emit("Draw2D");
            if (!OnDraw2D()) {
                CE_LOG(LogLevel::Error, "[Instance {}] OnDraw2D failed, shutting down instance", mInstanceID);
                mShouldExit = true;
                return 1;
            }
            mAnimatedTextureManager->Render();
            mRenderer->EndMode2D();

            mRenderer->ImGuiStartFrame();
            mACCS->system_manager.Update(mDeltaTime, ACCS::FrameUpdateType::ImGui, mACCS->world);
            mLogsWindow.DrawWindow();
            if (!OnImGui()) {
                CE_LOG(LogLevel::Error, "[Instance {}] OnImGui failed, shutting down instance", mInstanceID);
                mShouldExit = true;
                return 1;
            }
            mGameStateManager->Emit("DrawImgui");
            mRenderer->ImGuiEndFrame(mWindow->GetWindow());

            mRenderer->EndFrame(mWindow->GetWindow());
            mFontManager->Update();
        }

        Uint64 frame_end_counter = SDL_GetPerformanceCounter();
        mFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                     static_cast<float>(mPerformanceFrequency) * 1000.0f;

        if (mSettingsManager->Settings.maxFPS > 0) {
            const float target_frame_time_ms = 1000.0f / static_cast<float>(mSettingsManager->Settings.maxFPS);

            if (mFrameTime < target_frame_time_ms) {
                SDL_DelayPrecise(static_cast<Uint64>((target_frame_time_ms - mFrameTime) * 1000000.0f));

                frame_end_counter = SDL_GetPerformanceCounter();
                mFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                             static_cast<float>(mPerformanceFrequency) * 1000.0f;
            }
        }

        mDeltaTime = static_cast<float>(frame_end_counter - mLastFrameCounter) /
                     static_cast<float>(mPerformanceFrequency);
        mLastFrameCounter = frame_end_counter;

        mAnimatedTextureManager->Update(mDeltaTime);
        mTextureManager->UnloadPendingDeletions();
        return 0;
    }

    int BaseInstance::GetFPS() const {
        if (mDeltaTime <= 0.0f) {
            return 0;
        }
        return static_cast<int>(1.0f / mDeltaTime);
    }

    void BaseInstance::SetGameState(const std::string& state) {
        mGameStateManager->ChangeState(state);
    }

    const std::string& BaseInstance::GetGameState() const {
        return mGameStateManager->GetState();
    }

    void BaseInstance::HandleWindowEvents() {
        auto indices = SDL_Events::GetWindowEventIndices(mInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = SDL_Events::gEvents[i];

            switch (e.type) { // NOLINT
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                mShouldExit = true;
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                const auto window_mode = mWindow->GetWindowMode();
                if (mGameInfo->pauseRenderingWhenFocusLostInWindowedMode &&
                    window_mode != Common::Window::WindowMode::Fullscreen &&
                    window_mode != Common::Window::WindowMode::Borderless) {
                    mShouldRender = false;
                }

                // Rendering an unfocused fullscreen/borderless window is wasteful, and on
                // Windows the swapchain texture can come back nullptr, so pause rendering.
                if (window_mode == Common::Window::WindowMode::Fullscreen) {
                    mShouldRender = false;
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_FULLSCREEN");
                } else if (window_mode == Common::Window::WindowMode::Borderless) {
                    mShouldRender = false;
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_BORDERLESS");
                } else {
                    mGameStateManager->Emit("CE_WINDOW_FOCUS_LOST_WINDOWED");
                }

                if (mGameInfo->pauseUpdateWhenFocusLost) {
                    mWindowFocus = false;
                }
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                const auto window_mode = mWindow->GetWindowMode();
                if (mGameInfo->pauseUpdateWhenFocusLost) {
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

    void BaseInstance::ApplySettingsReload() {
        const int targetW = std::max(1, mSettingsManager->Settings.windowWidth);
        const int targetH = std::max(1, mSettingsManager->Settings.windowHeight);

        mWindow->SetWindowSize({targetW, targetH}, mSettingsManager->Settings.windowMode);

        if (mSettingsManager->Settings.windowMode != mWindow->GetWindowMode()) {
            mWindow->SetWindowMode(mSettingsManager->Settings.windowMode);
        }

        mRenderer->SetVSync(mSettingsManager->Settings.enableVSync);

        if (mAudioManager) {
            mAudioManager->SetMasterVolume(mSettingsManager->Settings.masterVolume);
            mAudioManager->SetMusicVolume(mSettingsManager->Settings.musicVolume);
            mAudioManager->SetSFXVolume(mSettingsManager->Settings.sfxVolume);
        }

        mGameStateManager->Emit("CE_SETTINGS_RELOAD");
    }
} // namespace CE