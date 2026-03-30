#include "engine/bootstrap.hpp"
#include "engine/renderers.hpp"
#include "engine/gameinfo.hpp"
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
        
        if (CE::Renderers::rendererName == "None") {
            CE::Renderers::renderer = RendererBackend::None;
        } else if (CE::Renderers::rendererName == "Software") {
            CE::Renderers::renderer = RendererBackend::Software;
        } else if (CE::Renderers::rendererName == "OpenGL") {
            CE::Renderers::renderer = RendererBackend::OpenGL;
        } else if (CE::Renderers::rendererName == "DirectX") {
            CE::Log(LogLevel::Fatal, "[Bootstrap] DirectX is unsupported");
            ShowError("[Bootstrap] DirectX is unsupported");
        } else if (CE::Renderers::rendererName == "Metal") {
            CE::Log(CE::LogLevel::Fatal, "[Bootstrap] Metal is unsupported");
            ShowError("[Bootstrap] Metal is not supported");
        } else if (CE::Renderers::rendererName == "Vulkan") {
            CE::Log(CE::LogLevel::Fatal, "[Bootstrap] Vulkan is unsupported");
            ShowError("[Bootstrap] Vulkan is not supported");
        } else {
            CE::Log(CE::LogLevel::Fatal, "[Bootstrap] Unknown renderer");
            ShowError("[Bootstrap] Unknown renderer");
            std::exit(4);
        }

        std::string window_title;
        if (CE::GameInfo::windowTitle.empty()) { // If a window title was not provided use the game name
            window_title = CE::GameInfo::gameNameString;
        } else {
            window_title = CE::GameInfo::windowTitle;
        }

        CE::Log(LogLevel::Info, "[Window] Window title: {}", CE::GameInfo::windowTitle);
        CE::Log(LogLevel::Info, "[Window] Window size: {} width, {} height", CE::GameInfo::windowWidth, CE::GameInfo::windowHeight);
        CE::Log(LogLevel::Info, "[Window] Window renderer: {}", CE::Renderers::rendererName);
        CE::Log(LogLevel::Info, "[Window] Max fps: {}", CE::GameInfo::maxFPS);

        SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
        if (CE::Renderers::renderer == RendererBackend::OpenGL) {
            windowFlags = static_cast<SDL_WindowFlags>(windowFlags | SDL_WINDOW_OPENGL);
        }

        CE::Global::gameWindow = SDL_CreateWindow(
            window_title.c_str(),
            CE::GameInfo::windowWidth,
            CE::GameInfo::windowHeight,
            windowFlags
        );

        CE::Renderers::Init(CE::Global::gameWindow);
    }
}
