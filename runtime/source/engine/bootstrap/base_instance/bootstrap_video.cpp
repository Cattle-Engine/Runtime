#include <memory>
#include <stdexcept>
#include <SDL3/SDL.h>

#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/common/window.hpp"

#include "ce_runtime/common/misc/error_box.hpp"
#include "ce_runtime/common/tracelog.hpp"
#include "ce_runtime/rendering/renderer.hpp"

namespace CE {
    int BaseInstance::Bootstrap_Video(CE::Renderer::GPUDeviceHandle gpu_device) {
        mRenderer = std::unique_ptr<CE::Renderer::IRenderer>(CE::Renderer::CreateRenderer(gpu_device->backend, mVFS.get()));
        mRenderer->PreWinInit();

        CE_LOG(CE::LogLevel::Info, "[Window] Window title: {}", mConf.window_title);
        CE_LOG(CE::LogLevel::Info, "[Window] Window size: {} width, {} height", mConf.window_width,
               mConf.window_height);
        CE_LOG(CE::LogLevel::Info, "[Window] Window renderer: {}", mConf.renderer_name);
        CE_LOG(CE::LogLevel::Info, "[Window] Max fps: {}", mConf.max_fps);

        SDL_WindowFlags windowFlags = 0;
        if (mRendererBackend == RendererBackend::OpenGL)
            windowFlags |= SDL_WINDOW_OPENGL;
        if (mConf.window_resizable)
            windowFlags |= SDL_WINDOW_RESIZABLE;

        try {
            mWindow = std::make_unique<Common::Window>(
                *mGameStateManager,
                *mVFS,
                mConf.window_title,
                Common::Window::WindowSize{mConf.window_width, mConf.window_height},
                windowFlags);
        } catch (const std::runtime_error& e) {
            ShowError("Failed to create game window :{");
            return 3;
        }

        if (mConf.window_mode == Common::Window::WindowMode::Fullscreen) {
            if (!mWindow->SetWindowMode(Common::Window::WindowMode::Fullscreen)) {
                mWindow->HideWindow(true);
                ShowError("Failed to set game window to fullscreen");
                return 4;
            }
        }

        if (gpu_device->backend == RendererBackend::None) {
            mWindow->HideWindow(true);
        }

        int rei = mRenderer->Init(mWindow->GetWindow(), mDebug, gpu_device);
        if (rei != 0) {
            return 4 + rei;
        }
        return 0;
    }
} // namespace CE