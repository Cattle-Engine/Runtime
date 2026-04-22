#include <format>
#include <memory>
#include <SDL3/SDL.h>

#include "engine/ui/debug_window.hpp"
#include "engine/instance.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/events.hpp"

namespace CE {
    Instance::Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice) {
        GLOBALINSTANCESCOUNTER ++;
        gInstanceID = GLOBALINSTANCESCOUNTER;

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

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating window & renderer", gInstanceID);
        int vis = CE::Bootstrap::Init_Video(gGameInfo, gDebug, gRenderer, gRendererBackend, gWindow, gVFS, gpudevice);
        if(vis != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Video setup returned with: {}", gInstanceID, vis));
        }
        gInstanceWindowID = SDL_GetWindowID(gWindow);

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating asset managers", gInstanceID);
        int ams = CE::Bootstrap::Init_AssetManagers(gTextureManager, gVFS, gRenderer);
        if (ams != 0) {
            throw std::runtime_error(
                std::format("[Instance {}] Failed to init asset managers: {}", gInstanceID, ams));
        }

        CE::Log(CE::LogLevel::Info, "[Instance {}] Creating input managers", gInstanceID);
        gKeyboardManger = std::make_unique<CE::Input::Keyboard>(gInstanceWindowID);
        gMouseManger = std::make_unique<CE::Input::Mouse>(gInstanceWindowID);

        gTextureManager->Load("welcome.gif", "test");

        int x, y;
        SDL_GetWindowSize(gWindow, &x, &y);

        Log(LogLevel::Info, "SDL_WINDOW SIZE: {}, {}", x, y);
    }

    bool Instance::ShouldExit() {
        return gShouldExit;
    }

	    int Instance::Update() {
	    if(gShouldExit) return 1;

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

        gRenderer->BeginFrame(gWindow);
        gRenderer->SetClearColor(252, 186, 3, 255);
        gTextureManager->DrawRot("test", 640, 360, 2.0f,{255, 255, 255, 255});

        // Start ImGui draw frame
        gRenderer->ImGuiStartFrame();

        CE::UI::DrawDebugUI(*gRenderer, *gTextureManager, *gGameInfo, *gKeyboardManger, *gMouseManger);

        gRenderer->ImGuiEndFrame(gWindow);

        gRenderer->EndFrame(gWindow);

        return 0;
    }

    int Instance::GetInstanceID() {
        return gInstanceID;
    }

    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        gTextureManager.reset();
        gRenderer->Shutdown(gWindow);
        SDL_DestroyWindow(gWindow);
    }
}
