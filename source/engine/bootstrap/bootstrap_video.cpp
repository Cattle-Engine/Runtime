#include "engine/bootstrap.hpp"
#include "engine/renderers.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/core.hpp"
#include <SDL3/SDL.h>

namespace CE::Bootstrap::Video {
    void Init() {
        if(SDL_Init(SDL_INIT_VIDEO) != 0) {
            CE::Log(CE::Fatal, "[Bootstrap] Failed to setup SDL for video: {}", SDL_GetError());
            ShowError("[Bootstrap] Unable to init game window :'(");
            std::exit(3);
        }
        
        std::string window_title;
        if (CE::GameInfo::windowTitle.empty()) { // If a window title was not provided use the game name
            window_title = CE::GameInfo::gameNameString;
        } else {
            window_title = CE::GameInfo::windowTitle;
        }

        CE::Global::gameWindow = SDL_CreateWindow(
            window_title.c_str(),
            CE::GameInfo::windowWidth,
            CE::GameInfo::windowHeight,
            SDL_WINDOWPOS_CENTERED
        );

        CE::Renderers::Init(CE::Global::gameWindow);
    }
}