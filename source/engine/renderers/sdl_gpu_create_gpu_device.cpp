#include <memory>
#include <SDL3/SDL.h>

#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {

    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo) {
        SDL_GPUDevice* gdevice;
        Renderer::GPUDevice deviceinfo;

        switch (backend) {
            case (RendererBackend::Vulkan):
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,
                    debugvideo, "vulkan");
                deviceinfo.backend = RendererBackend::Vulkan;
                break;
            
            case (RendererBackend::Metal):
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL,
                    debugvideo, "metal");
                deviceinfo.backend = RendererBackend::Metal;
                break;

            case (RendererBackend::DX12):
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL,
                    debugvideo, "direct3d12");
                deviceinfo.backend = RendererBackend::DX12;
                break;
            
            default:
                CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Got invalid RendererBackend");
                return nullptr;
                break;
        }
        deviceinfo.device = gdevice;

        return std::make_shared<Renderer::GPUDevice>(deviceinfo);
    }
}