#include "engine/bootstrap.hpp"
#include "engine/renderer.hpp"
#include "engine/gameinfo.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/core.hpp"
#include <SDL3/SDL.h>

namespace CE::Bootstrap {
    int Init_Video(std::unique_ptr<GameInfo>& gameinfo, bool debugvideo, 
        std::unique_ptr<CE::Renderer::IRenderer>& renderer, RendererBackend& backend, SDL_Window*& window,
        std::unique_ptr<VFS::VFS>& vfs) {
        if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            CE::Log(CE::Fatal, "[Bootstrap] Failed to setup SDL for video: {}", SDL_GetError());
            ShowError("[Bootstrap] Unable to init game window :'(");
            return 1;
        }
        
        if (gameinfo->rendererName == "None") {
            backend = RendererBackend::None;
        } else if (gameinfo->rendererName == "Software") {
            backend = RendererBackend::Software;
        } else if (gameinfo->rendererName == "OpenGL") {
            backend = RendererBackend::OpenGL;
        } else if (gameinfo->rendererName == "DX12") {
            backend = RendererBackend::DX12;
        } else if (gameinfo->rendererName == "Metal") {
            backend = RendererBackend::Metal;
        } else if (gameinfo->rendererName == "Vulkan") {
            backend = RendererBackend::Vulkan;
        } else {
            CE::Log(CE::LogLevel::Fatal, "[Bootstrap] Unknown renderer");
            return 2;
        }

        renderer = std::unique_ptr<CE::Renderer::IRenderer>(CE::Renderer::CreateRenderer(backend, vfs.get()));
        renderer->PreWinInit();

        std::string window_title;
        if (gameinfo->windowTitle.empty()) { // If a window title was not provided use the game name
            window_title = gameinfo->gameNameString;
        } else {
            window_title = gameinfo->windowTitle;
        }

        CE::Log(CE::LogLevel::Info, "[Window] Window title: {}", window_title);
        CE::Log(CE::LogLevel::Info, "[Window] Window size: {} width, {} height", gameinfo->windowWidth, gameinfo->windowHeight);
        CE::Log(CE::LogLevel::Info, "[Window] Window renderer: {}", gameinfo->rendererName);
        CE::Log(CE::LogLevel::Info, "[Window] Max fps: {}", gameinfo->maxFPS);

        SDL_WindowFlags windowFlags = 0;
        if (backend == RendererBackend::OpenGL) windowFlags |= SDL_WINDOW_OPENGL;
        if (gameinfo->fullscreen)              windowFlags |= SDL_WINDOW_FULLSCREEN;
        if (gameinfo->resizableWindow)         windowFlags |= SDL_WINDOW_RESIZABLE;
        windowFlags |= SDL_WINDOW_MAXIMIZED;

        window = SDL_CreateWindow(
            window_title.c_str(),
            gameinfo->windowWidth,
            gameinfo->windowHeight,
            windowFlags
        );

        if (window == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[Window] Failed to create game window: {}", SDL_GetError());
            ShowError("Failed to create game window :{");
            return 3;
        }

        int rei = renderer->Init(window, debugvideo);
        if (rei != 0) {
            return 4 + rei;
        }
        return 0;
    }
}
