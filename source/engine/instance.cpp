#include <format>
#include <memory>
#include <SDL3/SDL.h>
#include "SDL3_image/SDL_image.h"

#include "engine/ui/debug_window.hpp"
#include "engine/instance.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/fullscreen.hpp"
#include "engine/settings.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/events.hpp"

namespace CE {
    Instance::Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice) {
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

        gTextureManager->Load("welcome.gif", "test");
        gFontManager->Load("/roboto.ttf", "test");

        int x, y;
        SDL_GetWindowSize(gWindow, &x, &y);
    }

    bool Instance::ShouldExit() {
        return gShouldExit;
    }

    int Instance::Update() {
        if(gShouldExit) return 1;

        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        gKeyboardManger->Update();
        gMouseManger->Update();

        auto indices = CE::Events::GetWindowEventIndices(gInstanceWindowID);

        for (size_t i : indices) {
            const SDL_Event& e = CE::Events::gEvents[i];

            switch (e.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    gShouldExit = true;
                    break;
            }
        }

        if (gPendingSettingsReload) {
            ApplySettingsReload();
            gPendingSettingsReload = false;
        }

        gRenderer->SetClearColor(255, 255, 255, 255);

        gRenderer->BeginFrame(gWindow);

        gTextureManager->DrawRot("test", 640, 360, 0.0f,{255, 255, 255, 255});

        gFontManager->DrawEx("Goober", "test", 24, 50, 100, {0, 0, 0, 255});

        // Start ImGui draw frame
        gRenderer->ImGuiStartFrame();

        CE::UI::DrawDebugUI(
            *gRenderer,
            *gTextureManager,
            *gGameInfo,
            *gSettingsManager,
            *gKeyboardManger,
            *gMouseManger,
            GetFPS(),
            gDeltaTime,
            gFrameTime
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
                SDL_DelayPrecise(static_cast<Uint64>((target_frame_time_ms - gFrameTime) * 1000000.0f));
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
        gPendingSettingsReload = true;
    }

    void Instance::ApplySettingsReload() {
        if (gSettingsManager->Settings.fullscreen) {
            if (!CE::ApplyFullscreenMode(
                    gWindow,
                    gSettingsManager->Settings.windowWidth,
                    gSettingsManager->Settings.windowHeight)) {
                CE::Log(LogLevel::Error, "[Instance {}] Failed to apply fullscreen mode", gInstanceID);
            }
        } else  {
            SDL_SetWindowFullscreen(gWindow, false);
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

    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        gTextureManager.reset();
        gRenderer->Shutdown(gWindow);
        SDL_DestroyWindow(gWindow);
    }
}
