#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDL_GPU_Renderer::SetClearColour(float r, float g, float b, float a) {
        const bool looksLikeByteColor = (r > 1.0f) || (g > 1.0f) || (b > 1.0f) || (a > 1.0f);

        if (looksLikeByteColor) {
            r /= 255.0f;
            g /= 255.0f;
            b /= 255.0f;
            a /= 255.0f;
        }

        auto clamp01 = [](float v) {
            if (v < 0.0f)
                return 0.0f;
            if (v > 1.0f)
                return 1.0f;
            return v;
        };

        mClearColor = {clamp01(r), clamp01(g), clamp01(b), clamp01(a)};
    }

    int SDL_GPU_Renderer::Debug_GetVertCount() {
        return static_cast<int>(mVertCount);
    }

    int SDL_GPU_Renderer::Debug_GetIndexCount() {
        return static_cast<int>(mIndexCount);
    }

    int SDL_GPU_Renderer::Debug_GetTexIndexCount() {
        return static_cast<int>(mTexIndexCount);
    }

    int SDL_GPU_Renderer::Debug_GetTexVertCount() {
        return static_cast<int>(mTexIndexCount);
    }

    SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend backend, CE::Common::FS::VFS::VFS* vfs) {
        mBackend = backend;
        mVFS = vfs;
    }

    void SDL_GPU_Renderer::SetVSync(bool setting) {
        SDL_Window* window = SDL_GetWindowFromID(mWindowID);
        if (window == nullptr) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] SetVSync called before window was available");
            return;
        }

        const bool result = setting ? SDL_SetGPUSwapchainParameters(mDevice, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                                                    SDL_GPU_PRESENTMODE_VSYNC // VSync on
                                                                    )
                                    : SDL_SetGPUSwapchainParameters(mDevice, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                                                    SDL_GPU_PRESENTMODE_IMMEDIATE // VSync off
                                      );

        if (!result) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] Failed to set VSync to {}: {}", setting ? "enabled" : "disabled",
                   SDL_GetError());
        }
    }
}