#include <format>
#include <memory>
#include <SDL3/SDL.h>

#include "engine/instance.hpp"
#include "engine/bootstrap.hpp"
#include "engine/common/tracelog.hpp"


namespace CE {
    Instance::Instance(const char* data_file_name, bool debugmode) {
        GLOBALINSTANCESCOUNTER ++;
        gInstanceID = GLOBALINSTANCESCOUNTER;

        gVFS = std::make_unique<CE::VFS::VFS>();
        gGameInfo = std::make_unique<CE::GameInfo>();

        std::string fulldatapath = std::format("{}{}",SDL_GetBasePath(), data_file_name);

        CE::Log(CE::LogLevel::Info, "[Instance {}] Setting up game data", gInstanceID);
        int gds_return = Bootstrap::Init_GameData(gVFS, fulldatapath.c_str(), gDebug);
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
        int vis = CE::Bootstrap::Init_Video(gGameInfo, gDebug, gRenderer, gRendererBackend, gWindow, gVFS);
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
    }

    bool Instance::ShouldExit() {
        return gShouldExit;
    }

    int Instance::Update() {
    if(!gShouldExit) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                gShouldExit = true;
            }

            if (event.type >= SDL_EVENT_WINDOW_FIRST &&
                event.type <= SDL_EVENT_WINDOW_LAST) {

                if (event.window.windowID != gInstanceWindowID) {
                    continue;
                }
            }

            switch (event.type) {

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    
                    break;

                case SDL_EVENT_MOUSE_MOTION:

                    break;

                default:

                    break;
            }
        }

        gRenderer->BeginFrame(gWindow);
        gRenderer->SetClearColor(252, 186, 3, 255);

        gRenderer->EndFrame(gWindow);

        return 0;
    }
    }
    Instance::~Instance() {
        GLOBALINSTANCESCOUNTER--;
        gTextureManager.reset();
        gRenderer->Shutdown();
        SDL_DestroyWindow(gWindow);
    }
}
