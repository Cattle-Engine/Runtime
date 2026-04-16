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
                CE::Log(LogLevel::Fatal, "[SDL_GPU Device Creator] Got invalid RendererBackend");
                return nullptr;
                break;
        }
        deviceinfo.device = gdevice;

        CE::Log(LogLevel::Info,
            "[SDL_GPU Renderer] Backend given was: {}",
            static_cast<int>(deviceinfo.backend));
        
        if (deviceinfo.device == nullptr) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Device Creator] gDevice is, reason: {}", SDL_GetError());
        }

        return std::make_shared<Renderer::GPUDevice>(deviceinfo);
    }

    void DestroyGPUDevice(GPUDeviceHandle device) {
        SDL_WaitForGPUIdle(static_cast<SDL_GPUDevice*>(device->device));
        SDL_DestroyGPUDevice(static_cast<SDL_GPUDevice*>(device->device));
    }
}