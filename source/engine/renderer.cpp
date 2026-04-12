#include "engine/renderer.hpp"
#include "engine/renderers/sdl_gpu_renderer.hpp"

namespace CE::Renderer {
    CE::Renderer::IRenderer* CreateRenderer(RendererBackend backend) {
        switch (backend) {
            case RendererBackend::Vulkan:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::Vulkan);
                break;
            case RendererBackend::DX12:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::DX12);
                break;
            case RendererBackend::Metal:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::Metal);
                break;
            default:
                return nullptr;
        }
    }
}