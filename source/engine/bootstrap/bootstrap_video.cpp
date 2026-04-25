#include "engine/bootstrap/instance.hpp"
#include "engine/common/fullscreen.hpp"
#include "engine/renderer.hpp"
#include "engine/common/gameinfo.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include <SDL3/SDL.h>

namespace CE::Bootstrap {
    int Init_Video(std::unique_ptr<GameInfo>& gameinfo, const Settings::SettingsInfo& settings, bool debugvideo,
        std::unique_ptr<CE::Renderer::IRenderer>& renderer, RendererBackend& backend, SDL_Window*& window,
        std::unique_ptr<VFS::VFS>& vfs, Renderer::GPUDeviceHandle gpudevice) {

        renderer = std::unique_ptr<CE::Renderer::IRenderer>(CE::Renderer::CreateRenderer(gpudevice->backend, vfs.get()));
        renderer->PreWinInit();

        std::string window_title;
        if (gameinfo->windowTitle.empty()) { // If a window title was not provided use the game name
            window_title = gameinfo->gameNameString;
        } else {
            window_title = gameinfo->windowTitle;
        }

        CE::Log(CE::LogLevel::Info, "[Window] Window title: {}", window_title);
        CE::Log(CE::LogLevel::Info, "[Window] Window size: {} width, {} height", settings.windowWidth, settings.windowHeight);
        CE::Log(CE::LogLevel::Info, "[Window] Window renderer: {}", settings.rendererName);
        CE::Log(CE::LogLevel::Info, "[Window] Max fps: {}", settings.maxFPS);

        SDL_WindowFlags windowFlags = 0;
        if (backend == RendererBackend::OpenGL) windowFlags |= SDL_WINDOW_OPENGL;
        if (gameinfo->resizableWindow)          windowFlags |= SDL_WINDOW_RESIZABLE;

        window = SDL_CreateWindow(
            window_title.c_str(),
            settings.windowWidth,
            settings.windowHeight,
            windowFlags
        );

        if (window == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[Window] Failed to create game window: {}", SDL_GetError());
            ShowError("Failed to create game window :{");
            return 3;
        }

        if (settings.fullscreen) {
            if (!CE::ApplyFullscreenMode(window, settings.windowWidth, settings.windowHeight)) {
                CE::Log(CE::LogLevel::Fatal, "[Window] Failed to apply fullscreen mode: {}", SDL_GetError());
                return 4;
            }
        }

        int rei = renderer->Init(window, debugvideo, gpudevice);
        if (rei != 0) {
            return 4 + rei;
        }
        return 0;
    }
}
