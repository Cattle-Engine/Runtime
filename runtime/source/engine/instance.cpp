#include "engine/instance.hpp"

#include <exception>
#include <format>
#include <memory>
#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>

#include "engine/accs/system.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/common/fs/os_fs/directory_file_provider.hpp"
#include "engine/common/sdl_events.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/window.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/scripting/bindings/as_cubemap.hpp"
#include "engine/platforms.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/settings.hpp"
#include "engine/ui/debug_window.hpp"

namespace CE {
    Instance::Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice,
                       EngineArguements args)
        : mGameStateManager(mEventBus) {
        mProgramArguments = args;
        GLOBALINSTANCESCOUNTER++;
        mInstanceID = GLOBALINSTANCESCOUNTER;
        mDebug = debugmode;
        mPerformanceFrequency = SDL_GetPerformanceFrequency();
        mLastFrameCounter = SDL_GetPerformanceCounter();

        mVFS = std::make_unique<CE::Common::FS::VFS::VFS>();
        mGameInfo = std::make_unique<CE::GameInfo>();

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Setting up game data", mInstanceID);
        int gds_return = Bootstrap::Init_GameData(mVFS, data_file_path, mDebug);
        if (gds_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Gamedata mount returned with code {}", mInstanceID, gds_return));
        }

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating game info", mInstanceID);
        int gis_return = Bootstrap::Init_GameInfo(mVFS, mGameInfo, mDebug); 
        if (gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to get gameinfo with code: {}", mInstanceID, gis_return));
        }
        mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>("/config", 100,
                                                                       Platforms::GetConfigPath(mGameInfo->gameNameString));

        mSettingsManager = std::make_unique<CE::Settings::SettingsManager>(*mGameInfo, mInstanceID);
        mSettingsManager->SetInstance(*this);

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating window & renderer", mInstanceID);
        int vis = Bootstrap_Video(gpudevice);

        if (vis != 0) {
            throw std::runtime_error(std::format("[Instance {}] Video setup returned with: {}", mInstanceID, vis));
        }

        mRenderer->SetVSync(mSettingsManager->Settings.enableVSync);
        mInstanceWindowID = SDL_GetWindowID(mWindow->GetWindow());
        if (!mGameInfo->windowIcon.empty()) {
            mWindow->SetWindowIcon(mGameInfo->windowIcon);
        }

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating renderer resource managers", mInstanceID);
        int brrms = this->Bootstrap_RendererResourceManagers();
        if (brrms != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init renderer resource managers: {}", mInstanceID, brrms));
        }

        int aiam = this->Bootstrap_AssetImportersAndManagers();
        if (aiam != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init asset importers and managers {}", mInstanceID, aiam));
        }

        int bootstrap_input_return = Bootstrap_InputManagers();
        if (bootstrap_input_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init input managers", mInstanceID)
            );
        }

        try {
            CE_LOG(LogLevel::Info, "[Instance {}] SDL audio drivers:", mInstanceID);

            for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
                CE_LOG(LogLevel::Info, "  {}", SDL_GetAudioDriver(i));
            }

            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                CE_LOG(LogLevel::Error,
                    "[Instance {}] Failed to init SDL audio: {}",
                    mInstanceID, SDL_GetError());

                throw std::runtime_error("Failed to init SDL audio");
            }

            CE_LOG(LogLevel::Info,
                "[Instance {}] SDL audio driver: {}",
                mInstanceID, SDL_GetCurrentAudioDriver());

            CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating audio system", mInstanceID);
            
            SDL_AudioDeviceID audio_device_id = Core::Audio::AudioSystem::GetAudioDeviceID(mSettingsManager->Settings.audioDeviceName);
            mAudioSystem = std::make_unique<CE::Core::Audio::AudioSystem>(
                *mVFS, 
                mInstanceID, 
                static_cast<uint32_t>(audio_device_id), 
                Core::Audio::AudioSystem::IsAudioDeviceStereo(audio_device_id)
            );

            mAudioManager = std::make_unique<CE::Audio::Resources::AudioManager>(*mAudioSystem, *mVFS, mInstanceID);
            mAudioManager->SetMasterVolume(mSettingsManager->Settings.masterVolume);
            mAudioManager->SetMusicVolume(mSettingsManager->Settings.musicVolume);
            mAudioManager->SetSFXVolume(mSettingsManager->Settings.sfxVolume);
        } catch (const std::exception& e) {
            CE_LOG(CE::LogLevel::Error, "[Instance {}] Audio system failed to initialize: {}", mInstanceID, e.what());
            mAudioManager.reset();
            mAudioSystem.reset();
        }

        CE_LOG(LogLevel::Info, "[Instance {}] Creating ACCS", mInstanceID);
        Bootstrap_ACCS();
        CE_LOG(LogLevel::Info, "[Instance {}], Created ACCS", mInstanceID);

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
            mGameStateManager,
            mAudioManager.get(),
            mAudioSystem.get()
        );

        if (!mScriptingManager->Init()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format("[Instance {}] AngelScript initialization failed: {}", mInstanceID,
                                                 mScriptingManager->GetLastError()));
        }

        if (!mScriptingManager->RunStartup()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format("[Instance {}] AngelScript startup failed: {}", mInstanceID,
                                                 mScriptingManager->GetLastError()));
        }

        #ifdef CE_DEBUG
            mASDebuggerWindow = std::make_unique<UI::AngelscriptDebuggerWindow>(mScriptingManager->GetScriptDebugger(), *mScriptingManager, *mVFS);
        #endif


        mDebugWindow = std::make_unique<UI::DebugWindow>(
            *mRenderer, *mTextureManager, *mShaderManager, *mFontManager, *mGameInfo, *mSettingsManager,
            mAudioManager.get(), *mKeyboardManger, *this, *mMouseManger, mAudioSystem.get(), mASDebuggerWindow.get());

        mWindowFocus = true;
    }

    bool Instance::ShouldExit() {
        return mShouldExit;
    }

    int Instance::Update() {
        if (mShouldExit)
            return 1;

        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        mKeyboardManger->Update();
        mMouseManger->Update();
        mTextInputManager->Update();
        CE::Scripting::Bindings::ASCubemap::SetDefaultTextureManager(*mTextureManager);
        auto indices = CE::SDL_Events::GetWindowEventIndices(mInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = CE::SDL_Events::gEvents[i];

            switch (e.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                mShouldExit = true;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                    auto window_mode = mWindow->GetWindowMode();
                    if (mGameInfo->pauseRenderingWhenFocusLostInWindowedMode && window_mode != Common::Window::WindowMode::Fullscreen && window_mode != Common::Window::WindowMode::Borderless) {
                        mShouldRender = false;
                    }

                    // Due to it being wasteful on the GPU to render the window when its not even visible
                    // and on windows the swapchain texture returns nullptr renderering is paused
                    if (window_mode == Common::Window::WindowMode::Fullscreen) {
                        mShouldRender = false;
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_FULLSCREEN");
                    } else if (window_mode == Common::Window::WindowMode::Borderless) {
                        mShouldRender = false;
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_BORDERLESS");
                    } else {
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_WINDOWED");
                    }

                    if (mGameInfo->pauseUpdateWhenFocusLost) {
                        mWindowFocus = false;
                    }

                    break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                    auto window_mode = mWindow->GetWindowMode();
                    if (mGameInfo->pauseUpdateWhenFocusLost) {
                        mWindowFocus = true;
                    }
                    mShouldRender = true;
                    if (window_mode == Common::Window::WindowMode::Fullscreen) {
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_GAINED_FULLSCREEN");
                    } else if (window_mode == Common::Window::WindowMode::Borderless) {
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_GAINED_BORDERLESS");
                    } else {
                        mGameStateManager.Emit("CE_WINDOW_FOCUS_GAINED_WINDOWED");
                    }
                    break;
                }
            }
        }

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
        mACCS->system_manager.Update(GetDeltaTime(), ACCS::FrameUpdateType::Logic, mACCS->world);
        mGameStateManager.Emit("Update");
        if (mShouldRender) {
            int bfr = mRenderer->BeginFrame(mWindow->GetWindow());
            if (bfr != 0) {
                return 1;
            }
            mRenderer->BeginMode3D();
            mACCS->system_manager.Update(GetDeltaTime(), ACCS::FrameUpdateType::Render3D, mACCS->world);
            mGameStateManager.Emit("Draw3D");
            mRenderer->EndMode3D();

            mRenderer->BeginMode2D();
            mACCS->system_manager.Update(GetDeltaTime(), ACCS::FrameUpdateType::Render2D, mACCS->world);
            mGameStateManager.Emit("Draw2D");
            if (!mScriptingManager->RunUpdate()) {
                ShowError(mScriptingManager->GetLastError());
                CE_LOG(LogLevel::Error, "[Instance {}] AngelScript update failed, shutting down instance", mInstanceID);
                mShouldExit = true;
                return 1;
            }
            mAnimatedTextureManager->Render();
            mRenderer->EndMode2D();

            mRenderer->ImGuiStartFrame();
            mACCS->system_manager.Update(GetDeltaTime(), ACCS::FrameUpdateType::ImGui, mACCS->world);
            mDebugWindow->Draw();
            if (mASDebuggerWindow) {
                mASDebuggerWindow->DrawWindow();
            }

            if (!mScriptingManager->RunImGui()) {
                ShowError(mScriptingManager->GetLastError());
                CE_LOG(LogLevel::Error, "[Instance {}] AngelScript ImGui update failed, shutting down instance", mInstanceID);
                mShouldExit = true;
                return 1;
            }

            mGameStateManager.Emit("DrawImgui");

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

        mDeltaTime =
            static_cast<float>(frame_end_counter - mLastFrameCounter) / static_cast<float>(mPerformanceFrequency);

        mLastFrameCounter = frame_end_counter;
        mAnimatedTextureManager->Update(mDeltaTime);
        mTextureManager->UnloadPendingDeletions();
        return 0;
    }

    int Instance::GetInstanceID() {
        return mInstanceID;
    }

    void Instance::SetGameState(const std::string& state) {
        mGameStateManager.ChangeState(state);
    }

    const std::string& Instance::GetGameState() const {
        return mGameStateManager.GetState();
    }

    CE::Core::EventBus& Instance::GetEventBus() {
        return mEventBus;
    }

    CE::Core::GameState::GameStateManager& Instance::GetGameStateManager() {
        return mGameStateManager;
    }

    ACCS::ACCS& Instance::GetACCS() {
        return *mACCS;
    }

    float Instance::GetDeltaTime() const {
        return mDeltaTime;
    }

    float Instance::GetFrameTime() const {
        return mFrameTime;
    }

    int Instance::GetFPS() const {
        if (mDeltaTime <= 0.0f)
            return 0.0f;
        return 1.0f / mDeltaTime;
    }

    void Instance::ReloadSettings() {
        CE_LOG(LogLevel::Debug, "[Instance {}] ReloadSettings called", mInstanceID);
        mPendingSettingsReload = true;
    }

    void Instance::ApplySettingsReload() {
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

        mGameStateManager.Emit("CE_SETTINGS_RELOAD");
    }

    void Instance::Exit() {
        mShouldExit = true;
    }

    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        mScriptingManager.reset();
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
        mRenderer->Shutdown(mWindow->GetWindow());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
} // namespace CE
