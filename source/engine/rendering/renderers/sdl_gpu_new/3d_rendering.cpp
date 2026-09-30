#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDLGPURenderer::BeginMode2D() {
        mMode2DActive = true;
        mMode3DActive = false;
    }

    void SDLGPURenderer::EndMode2D() {
        mMode2DActive = false;
    }

    void SDLGPURenderer::BeginMode3D() {
        mMode3DActive = true;
        mMode2DActive = false;
    }

    void SDLGPURenderer::EndMode3D() {
        mMode3DActive = false;
    }

    void SDLGPURenderer::SetRenderSize(glm::vec2 size) {
        if (size.x <= 0.0f) {
            size.x = 1.0f;
        }
        if (size.y <= 0.0f) {
            size.y = 1.0f;
        }

        pRenderSize = size;
    }
}
