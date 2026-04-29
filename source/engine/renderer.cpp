#include "engine/renderer.hpp"
#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/fs/vfs.hpp"
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

    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo) {
        switch (backend) {
            case RendererBackend::Vulkan:
                return CE::Renderer::SDL_GPU_Renderer::CreateGPUDevice(backend, debugvideo);
                break;
            case RendererBackend::DX12:
                return CE::Renderer::SDL_GPU_Renderer::CreateGPUDevice(backend, debugvideo);
                break;
            case RendererBackend::Metal:
                return CE::Renderer::SDL_GPU_Renderer::CreateGPUDevice(backend, debugvideo);
            default:
                return nullptr;
        }
    }

    void DestroyGPUDevice(GPUDeviceHandle device) {
        switch (device->backend) {
            case RendererBackend::Vulkan:
            case RendererBackend::Metal:
            case RendererBackend::DX12:
                CE::Renderer::SDL_GPU_Renderer::DestroyGPUDevice(device);
            default:
                break;
        }
    }
}
