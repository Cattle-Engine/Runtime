#include "engine/instance.hpp"

#include <format>
#include <memory>

#include <SDL3/SDL.h>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/common/sdl_events.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/window.hpp"
#include "engine/platforms.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/settings.hpp"
#include "engine/ui/debug_window.hpp"


namespace CE {
    Instance::Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice,
                       EngineArguements args)
        : gGameStateManager(gEventBus) {
        gProgramArguments = args;
        GLOBALINSTANCESCOUNTER++;
        gInstanceID = GLOBALINSTANCESCOUNTER;
        gDebug = debugmode;
        gPerformanceFrequency = SDL_GetPerformanceFrequency();
        gLastFrameCounter = SDL_GetPerformanceCounter();

        mVFS = std::make_unique<CE::VFS::VFS>();
        mGameInfo = std::make_unique<CE::GameInfo>();

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Setting up game data", gInstanceID);
        int gds_return = Bootstrap::Init_GameData(mVFS, data_file_path, gDebug);
        if (gds_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Gamedata mount returned with code {}", gInstanceID, gds_return));
        }

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating game info", gInstanceID);
        int gis_return = Bootstrap::Init_GameInfo(mVFS, mGameInfo, gDebug); 
        if (gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to get gameinfo with code: {}", gInstanceID, gis_return));
        }
        mVFS->MountFolder(Platforms::GetConfigPath(mGameInfo->gameNameString).c_str(), "/config", LoadMode::OnDemand,
                          100);

        mSettingsManager = std::make_unique<CE::Settings::SettingsManager>(*mGameInfo, gInstanceID);
        mSettingsManager->SetInstance(*this);

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating window & renderer", gInstanceID);
        int vis = Bootstrap_Video(gpudevice);

        if (vis != 0) {
            throw std::runtime_error(std::format("[Instance {}] Video setup returned with: {}", gInstanceID, vis));
        }

        mRenderer->SetVSync(mSettingsManager->Settings.enableVSync);
        gInstanceWindowID = SDL_GetWindowID(mWindow->GetWindow());
        if (!mGameInfo->windowIcon.empty()) {
            mWindow->SetWindowIcon(mGameInfo->windowIcon);
        }

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating renderer resource managers", gInstanceID);
        int brrms = this->Bootstrap_RendererResourceManagers();
        if (brrms != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init renderer resource managers: {}", gInstanceID, brrms));
        }

        int aiam = this->Bootstrap_AssetImportersAndManagers();
        if (aiam != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init asset importers and managers {}", gInstanceID, aiam));
        }

        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating input managers", gInstanceID);
        mKeyboardManger = std::make_unique<CE::Input::Keyboard>(gInstanceWindowID);
        mMouseManger = std::make_unique<CE::Input::Mouse>(gInstanceWindowID);

        try {
            CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating audio system", gInstanceID);
            mAudioSystem = std::make_unique<CE::Core::Audio::AudioSystem>(
                *mVFS, gInstanceID, static_cast<uint32_t>(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), true);

            mAudioManager = std::make_unique<CE::Audio::Resources::AudioManager>(*mAudioSystem, *mVFS, gInstanceID);
            mAudioManager->SetMasterVolume(mSettingsManager->Settings.masterVolume);
            mAudioManager->SetMusicVolume(mSettingsManager->Settings.musicVolume);
            mAudioManager->SetSFXVolume(mSettingsManager->Settings.sfxVolume);
        } catch (const std::exception& e) {
            CE_LOG(CE::LogLevel::Error, "[Instance {}] Audio system failed to initialize: {}", gInstanceID, e.what());
            mAudioManager.reset();
            mAudioSystem.reset();
        }

        mScriptingManager = std::make_unique<CE::Scripting::Runtime>(
            *mVFS, *mGameInfo, 
            *mSettingsManager, *this, 
            *mRenderer, *mTextureManager, 
            *mShaderManager, *gFontManager, 
            *mGPUMeshManager, *mMaterialManager, 
            *gAnimatedTextureManager, *mKeyboardManger,
            *mMouseManger, *mRendererResourcesNameRegistry, 
            gProgramArguments.OutputDebugASInfo,
            gProgramArguments.OutputDebugASInfoPath, 
            *mWindow,mAudioManager.get()
        );

        if (!mScriptingManager->Init()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format("[Instance {}] AngelScript initialization failed: {}", gInstanceID,
                                                 mScriptingManager->GetLastError()));
        }

        if (!mScriptingManager->RunStartup()) {
            ShowError(mScriptingManager->GetLastError());
            throw std::runtime_error(std::format("[Instance {}] AngelScript startup failed: {}", gInstanceID,
                                                 mScriptingManager->GetLastError()));
        }
        gWindowFocus = true;
    }

    bool Instance::ShouldExit() {
        return gShouldExit;
    }

    int Instance::Update() {
        if (gShouldExit)
            return 1;

        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        mKeyboardManger->Update();
        mMouseManger->Update();

        auto indices = CE::SDL_Events::GetWindowEventIndices(gInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = CE::SDL_Events::gEvents[i];

            switch (e.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                gShouldExit = true;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                    auto window_mode = mWindow->GetWindowMode();
                    if (mGameInfo->pauseRenderingWhenFocusLostInWindowedMode && window_mode != Common::Window::WindowMode::Fullscreen && window_mode != Common::Window::WindowMode::Borderless) {
                        gShouldRender = false;
                    }

                    // Due to it being wasteful on the GPU to render the window when its not even visible
                    // and on windows the swapchain texture returns nullptr renderering is paused
                    if (window_mode == Common::Window::WindowMode::Fullscreen) {
                        gShouldRender = false;
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_FULLSCREEN");
                    } else if (window_mode == Common::Window::WindowMode::Borderless) {
                        gShouldRender = false;
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_BORDERLESS");
                    } else {
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_LOST_WINDOWED");
                    }

                    if (mGameInfo->pauseUpdateWhenFocusLost) {
                        gWindowFocus = false;
                    }

                    break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                    auto window_mode = mWindow->GetWindowMode();
                    if (mGameInfo->pauseUpdateWhenFocusLost) {
                        gWindowFocus = true;
                    }
                    gShouldRender = true;
                    if (window_mode == Common::Window::WindowMode::Fullscreen) {
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_GAIMED_FULLSCREEN");
                    } else if (window_mode == Common::Window::WindowMode::Borderless) {
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_GAINED_BORDERLESS");
                    } else {
                        gGameStateManager.Emit("CE_WINDOW_FOCUS_GAINED_WINDOWED");
                    }
                    break;
                }
            }
        }

        if (gPendingSettingsReload) {
            ApplySettingsReload();
            gPendingSettingsReload = false;
        }

        if (!gWindowFocus) {
            gLastFrameCounter = SDL_GetPerformanceCounter();
            gDeltaTime = 0.0f;
            return 0;
        }

        mRenderer->SetClearColor(255, 255, 255, 255);
        gGameStateManager.Emit("Update");
        if (gShouldRender) {
            int bfr = mRenderer->BeginFrame(mWindow->GetWindow());
            if (bfr != 0) {
                return 1;
            }
            mRenderer->BeginMode3D();
            gGameStateManager.Emit("Draw3D");
            mRenderer->EndMode3D();

            mRenderer->BeginMode2D();
            gGameStateManager.Emit("Draw2D");
            if (!mScriptingManager->RunUpdate()) {
                ShowError(mScriptingManager->GetLastError());
                CE_LOG(LogLevel::Error, "[Instance {}] AngelScript update failed, shutting down instance", gInstanceID);
                gShouldExit = true;
                return 1;
            }
            gAnimatedTextureManager->Render();
            mRenderer->EndMode2D();

            mRenderer->ImGuiStartFrame();
            gDebugWindow.Draw(*mRenderer, *mTextureManager, *mShaderManager, *gFontManager, *mGameInfo,
                              *mSettingsManager, mAudioManager.get(), *mKeyboardManger, *this, *mMouseManger,
                              this->GetFPS(), this->GetDeltaTime(), this->GetFrameTime());
            mRenderer->ImGuiEndFrame(mWindow->GetWindow());

            mRenderer->EndFrame(mWindow->GetWindow());
            gFontManager->Update();
        }

        Uint64 frame_end_counter = SDL_GetPerformanceCounter();
        gFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                     static_cast<float>(gPerformanceFrequency) * 1000.0f;

        if (mSettingsManager->Settings.maxFPS > 0) {
            const float target_frame_time_ms = 1000.0f / static_cast<float>(mSettingsManager->Settings.maxFPS);

            if (gFrameTime < target_frame_time_ms) {
                SDL_DelayPrecise(static_cast<Uint64>((target_frame_time_ms - gFrameTime) * 1000000.0f));

                frame_end_counter = SDL_GetPerformanceCounter();
                gFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                             static_cast<float>(gPerformanceFrequency) * 1000.0f;
            }
        }

        gDeltaTime =
            static_cast<float>(frame_end_counter - gLastFrameCounter) / static_cast<float>(gPerformanceFrequency);

        gLastFrameCounter = frame_end_counter;
        gAnimatedTextureManager->Update(gDeltaTime);
        mTextureManager->UnloadPendingDeletions();
        return 0;
    }

    int Instance::GetInstanceID() {
        return gInstanceID;
    }

    void Instance::SetGameState(const std::string& state) {
        gGameStateManager.ChangeState(state);
    }

    const std::string& Instance::GetGameState() const {
        return gGameStateManager.GetState();
    }

    CE::Core::EventBus& Instance::GetEventBus() {
        return gEventBus;
    }

    CE::Core::GameState::GameStateManager& Instance::GetGameStateManager() {
        return gGameStateManager;
    }

    float Instance::GetDeltaTime() const {
        return gDeltaTime;
    }

    float Instance::GetFrameTime() const {
        return gFrameTime;
    }

    int Instance::GetFPS() const {
        if (gDeltaTime <= 0.0f)
            return 0.0f;
        return 1.0f / gDeltaTime;
    }

    void Instance::ReloadSettings() {
        CE_LOG(LogLevel::Debug, "[Instance {}] ReloadSettings called", gInstanceID);
        gPendingSettingsReload = true;
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
    }

    void Instance::Exit() {
        gShouldExit = true;
    }

    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        mScriptingManager.reset();
        mAudioManager.reset();
        mAudioSystem.reset();
        g3DModelImporter.reset();
        gFontManager.reset();
        gAnimatedTextureManager.reset();
        mShaderManager.reset();
        mModelRenderer.reset();
        mGPUMeshManager.reset();
        mMaterialManager.reset();
        mTextureManager.reset();
        mRenderer->Shutdown(mWindow->GetWindow());
    }
} // namespace CE
