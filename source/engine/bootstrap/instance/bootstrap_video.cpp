#include <memory>
#include <stdexcept>
#include <SDL3/SDL.h>

#include "engine/instance.hpp"
#include "engine/common/window.hpp"

#include "engine/common/misc/error_box.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE {
    int Instance::Bootstrap_Video(CE::Renderer::GPUDeviceHandle gpu_device) {
        mRenderer = std::unique_ptr<CE::Renderer::IRenderer>(CE::Renderer::CreateRenderer(gpu_device->backend, mVFS.get()));
        mRenderer->PreWinInit();

        std::string window_title;
        if (mGameInfo->windowTitle.empty()) { // If a window title was not provided use the game name
            window_title = mGameInfo->gameNameString;
        } else {
            window_title = mGameInfo->windowTitle;
        }
        
        auto settings = mSettingsManager->Settings;

        CE_LOG(CE::LogLevel::Info, "[Window] Window title: {}", window_title);
        CE_LOG(CE::LogLevel::Info, "[Window] Window size: {} width, {} height", settings.windowWidth,
               settings.windowHeight);
        CE_LOG(CE::LogLevel::Info, "[Window] Window renderer: {}", settings.rendererName);
        CE_LOG(CE::LogLevel::Info, "[Window] Max fps: {}", settings.maxFPS);

        SDL_WindowFlags windowFlags = 0;
        if (gRendererBackend == RendererBackend::OpenGL)
            windowFlags |= SDL_WINDOW_OPENGL;
        if (mGameInfo->resizableWindow)
            windowFlags |= SDL_WINDOW_RESIZABLE;

        try {
            mWindow = std::make_unique<Common::Window>(*mVFS, window_title, Common::Window::WindowSize{settings.windowWidth, settings.windowHeight}, windowFlags);                                                                                                                                                                                      
        } catch (const std::runtime_error& e) {
            ShowError("Failed to create game window :{");
            return 3;
        }

        if (settings.windowMode == Common::Window::WindowMode::Fullscreen) {
            if (!mWindow->SetWindowMode(Common::Window::WindowMode::Fullscreen)) {
                mWindow->HideWindow(true);
                ShowError("Failed to set game window to fullscreen");
                return 4;
            }
        }

        int rei = mRenderer->Init(mWindow->GetWindow(), gDebug, gpu_device);
        if (rei != 0) {
            return 4 + rei;
        }
        return 0;
    }
} // namespace CE::Bootstrap
