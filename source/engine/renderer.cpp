#include "engine/renderer.hpp"
#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/vfs.hpp"
#include "memory"

namespace CE::Renderer {
    CE::Renderer::IRenderer* CreateRenderer(RendererBackend backend, VFS::VFS* vfs) {
        switch (backend) {
            case RendererBackend::Vulkan:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::Vulkan, 
                    vfs);
                break;
            case RendererBackend::DX12:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::DX12,
                vfs);
                break;
            case RendererBackend::Metal:
                return new CE::Renderer::SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend::Metal,
                vfs);
                break;
            default:
                return nullptr;
        }
    }
}
