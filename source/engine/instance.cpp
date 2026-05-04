#include <format>
#include <memory>
#include <SDL3/SDL.h>
#include "SDL3_image/SDL_image.h"

#include "engine/ui/debug_window.hpp"
#include "engine/instance.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/fullscreen.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/settings.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/sdl_events.hpp"

namespace CE {
    Instance::Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice)
        : gGameStateManager(gEventBus) {
        GLOBALINSTANCESCOUNTER ++;
        gInstanceID = GLOBALINSTANCESCOUNTER;
        gDebug = debugmode;
        gPerformanceFrequency = SDL_GetPerformanceFrequency();
        gLastFrameCounter = SDL_GetPerformanceCounter();

        gVFS = std::make_unique<CE::VFS::VFS>();
        gGameInfo = std::make_unique<CE::GameInfo>();

        CE::Log(CE::LogLevel::Info, "[Instance {}] Setting up game data", gInstanceID);
        int gds_return = Bootstrap::Init_GameData(gVFS, data_file_path, gDebug);
        if(gds_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Gamedata mount returned with code {}", gInstanceID, gds_return));
        }

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating game info", gInstanceID);
        int gis_return = Bootstrap::Init_GameInfo(gVFS, gGameInfo, gDebug);
        if(gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to get gameinfo with code: {}", gInstanceID, gis_return));
        }

        gSettingsManager = std::make_unique<CE::Settings::SettingsManager>(*gGameInfo, gInstanceID);
        gSettingsManager->SetInstance(*this);

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating window & renderer", gInstanceID);
        int vis = CE::Bootstrap::Init_Video(
            gGameInfo,
            gSettingsManager->Settings,
            gDebug,
            gRenderer,
            gRendererBackend,
            gWindow,
            gVFS,
            gpudevice
        );
        if(vis != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Video setup returned with: {}", gInstanceID, vis));
        }

        gRenderer->SetVSync(gSettingsManager->Settings.enableVSync);
        gInstanceWindowID = SDL_GetWindowID(gWindow);
        if (!gGameInfo->windowIcon.empty()) {
            SetWindowIcon(gGameInfo->windowIcon);
        }
        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating asset managers", gInstanceID);
        int ams = CE::Bootstrap::Init_AssetManagers(gTextureManager, gVFS, gRenderer, gFontManager, gInstanceID);
        if (ams != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init asset managers: {}", gInstanceID, ams));
        }

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating input managers", gInstanceID);
        gKeyboardManger = std::make_unique<CE::Input::Keyboard>(gInstanceWindowID);
        gMouseManger = std::make_unique<CE::Input::Mouse>(gInstanceWindowID);
        
        gScriptingManager = std::make_unique<CE::Scripting::Runtime>(
            *gVFS,
            *gGameInfo,
            *gSettingsManager,
            *this,
            *gRenderer,
            *gTextureManager,
            *gFontManager,
            *gKeyboardManger,
            *gMouseManger
        );
        if (!gScriptingManager->Initialize()) {
            ShowError(gScriptingManager->GetLastError());
            throw std::runtime_error(
                std::format("[Instance {}] AngelScript initialization failed: {}", gInstanceID, gScriptingManager->GetLastError()));
        }

        if (!gScriptingManager->RunStartup()) {
            ShowError(gScriptingManager->GetLastError());
            throw std::runtime_error(
                std::format("[Instance {}] AngelScript startup failed: {}", gInstanceID, gScriptingManager->GetLastError()));
        }
        gWindowFocus = true;
    }

    bool Instance::ShouldExit() {
        return gShouldExit;
    }

    int Instance::Update() {
        if (gShouldExit) return 1;

        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        gKeyboardManger->Update();
        gMouseManger->Update();

        auto indices = CE::SDL_Events::GetWindowEventIndices(gInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = CE::SDL_Events::gEvents[i];

            switch (e.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    gShouldExit = true;
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    gWindowFocus = false;
                    break;
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    gWindowFocus = true;
                    break;
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

        gRenderer->SetClearColor(255, 255, 255, 255);
        
        int bfr = gRenderer->BeginFrame(gWindow);
        if (bfr != 0) {
            return 1;
        } 

        gGameStateManager.Emit("Draw");

        gGameStateManager.Emit("Update");
        if (!gScriptingManager->RunUpdate()) {
            ShowError(gScriptingManager->GetLastError());
            CE::Log(LogLevel::Error, "[Instance {}] AngelScript update failed, shutting down instance", gInstanceID);
            gShouldExit = true;
            return 1;
        }

        gRenderer->ImGuiStartFrame();

        gDebugWindow.Draw(
            *gRenderer,
            *gTextureManager,
            *gFontManager,
            *gGameInfo,
            *gSettingsManager,
            *gKeyboardManger,
            *this,
            *gMouseManger,
            this->GetFPS(),
            this->GetDeltaTime(),
            this->GetFrameTime()
        );

        gRenderer->ImGuiEndFrame(gWindow);

        gFontManager->Update();
        gRenderer->EndFrame(gWindow);

        Uint64 frame_end_counter = SDL_GetPerformanceCounter();
        gFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                    static_cast<float>(gPerformanceFrequency) * 1000.0f;

        if (gSettingsManager->Settings.maxFPS > 0) {
            const float target_frame_time_ms = 1000.0f /
                static_cast<float>(gSettingsManager->Settings.maxFPS);

            if (gFrameTime < target_frame_time_ms) {
                SDL_DelayPrecise(static_cast<Uint64>(
                    (target_frame_time_ms - gFrameTime) * 1000000.0f));

                frame_end_counter = SDL_GetPerformanceCounter();
                gFrameTime = static_cast<float>(frame_end_counter - frame_start_counter) /
                            static_cast<float>(gPerformanceFrequency) * 1000.0f;
            }
        }

        gDeltaTime = static_cast<float>(frame_end_counter - gLastFrameCounter) /
                    static_cast<float>(gPerformanceFrequency);

        gLastFrameCounter = frame_end_counter;

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
        if (gDeltaTime <= 0.0f) return 0.0f;
        return 1.0f / gDeltaTime;
    }

    void Instance::ReloadSettings() {
        CE::Log(LogLevel::Debug,"[Instance {}] ReloadSettings called", gInstanceID);
        gPendingSettingsReload = true;
    }

    void Instance::ApplySettingsReload() {
        const int targetW = std::max(1, gSettingsManager->Settings.windowWidth);
        const int targetH = std::max(1, gSettingsManager->Settings.windowHeight);

        if (gSettingsManager->Settings.fullscreen) {
            if (!CE::ApplyFullscreenMode(
                    gWindow,
                    targetW,
                    targetH)) {
                CE::Log(LogLevel::Error, "[Instance {}] Failed to apply fullscreen mode", gInstanceID);
            }
        } else  {
            // Ensure we exit fullscreen before resizing.
            if (!SDL_SetWindowFullscreenMode(gWindow, nullptr)) {
                CE::Log(LogLevel::Warn, "[Instance {}] SDL_SetWindowFullscreenMode(nullptr) failed: {}", gInstanceID, SDL_GetError());
            }
            if (!SDL_SetWindowFullscreen(gWindow, false)) {
                CE::Log(LogLevel::Warn, "[Instance {}] SDL_SetWindowFullscreen(false) failed: {}", gInstanceID, SDL_GetError());
            }

            if (!SDL_SetWindowSize(gWindow, targetW, targetH)) {
                CE::Log(LogLevel::Warn, "[Instance {}] SDL_SetWindowSize({}x{}) failed: {}", gInstanceID, targetW, targetH, SDL_GetError());
            }
        }

        gRenderer->SetVSync(gSettingsManager->Settings.enableVSync);
    }

    void Instance::SetWindowIcon(std::string path) {
        uint64_t sz = 0;
        if (!gVFS->GetFileSize(path.c_str(), sz) || sz == 0) {
            CE::Log(LogLevel::Error, "[Instance {}] VFS could not stat '{}' (missing or empty)", gInstanceID, path);
        }

        VirtualFile* vf = gVFS->OpenFile(path.c_str());
        if (!vf) {
            CE::Log(LogLevel::Error, "[Instance {}] VFS could not open '{}'", gInstanceID ,path);
        }

        std::vector<uint8_t> fileBytes((size_t)sz);
        gVFS->ReadFile(vf, fileBytes.data(), fileBytes.size());
        gVFS->CloseFile(vf);

        SDL_IOStream* mem = SDL_IOFromConstMem(fileBytes.data(), fileBytes.size());
        if (!mem) {
            CE::Log(LogLevel::Error, "[Instance {}] SDL_IOFromConstMem failed: {}", gInstanceID,SDL_GetError());
        }

        SDL_Surface* surface = IMG_Load_IO(mem, true); 
        if (!surface) {
            CE::Log(LogLevel::Error, "[Instance {}] IMG_Load_IO failed for '{}': {}", gInstanceID,path, SDL_GetError());
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE::Log(LogLevel::Error, "[Instance {}] SDL_ConvertSurface failed: {}", gInstanceID,SDL_GetError());
        }

        SDL_SetWindowIcon(gWindow, converted);
        SDL_DestroySurface(converted);
    }

    void Instance::Exit() {
        gShouldExit = true;
    }

    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        gTextureManager.reset();
        gRenderer->Shutdown(gWindow);
        SDL_DestroyWindow(gWindow);
    }
}
