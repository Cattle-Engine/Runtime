
#include <SDL3/SDL.h>

#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {

    static void LogAvailableGPUDrivers() {
        const int driverCount = SDL_GetNumGPUDrivers();
        if (driverCount <= 0) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Device Creator] SDL reports 0 built-in GPU drivers");
            return;
        }

        CE_LOG(LogLevel::Info, "[SDL_GPU Device Creator] Built-in GPU drivers ({}):", driverCount);
        for (int i = 0; i < driverCount; ++i) {
            const char* driver = SDL_GetGPUDriver(i);
            CE_LOG(LogLevel::Info, "  - {}", (driver ? driver : "(null)"));
        }
    }

    static bool HasGPUDriver(const char* name) {
        const int driverCount = SDL_GetNumGPUDrivers();
        for (int i = 0; i < driverCount; ++i) {
            const char* driver = SDL_GetGPUDriver(i);
            if (driver && name && SDL_strcasecmp(driver, name) == 0) {
                return true;
            }
        }
        return false;
    }

    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo) {
        SDL_GPUDevice* gdevice = nullptr;
        Renderer::GPUDevice deviceinfo;

        static bool sLoggedDrivers = false;
        if (!sLoggedDrivers) {
            LogAvailableGPUDrivers();
            sLoggedDrivers = true;
        }

        switch (backend) {
            case (RendererBackend::Vulkan):
                if (!HasGPUDriver("vulkan")) {
                    CE_LOG(LogLevel::Fatal,
                        "[SDL_GPU Device Creator] SDL was built/packaged without a 'vulkan' GPU driver.");
                    return nullptr;
                }
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,
                    debugvideo, "vulkan");
                deviceinfo.backend = RendererBackend::Vulkan;
                break;
            
            case (RendererBackend::Metal):
                if (!HasGPUDriver("metal")) {
                    CE_LOG(LogLevel::Fatal,
                        "[SDL_GPU Device Creator] SDL was built/packaged without a 'metal' GPU driver.");
                    return nullptr;
                }
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL,
                    debugvideo, "metal");
                deviceinfo.backend = RendererBackend::Metal;
                break;

            case (RendererBackend::DX12):
                if (!HasGPUDriver("direct3d12")) {
                    CE_LOG(LogLevel::Fatal,
                        "[SDL_GPU Device Creator] SDL was built/packaged without a 'direct3d12' GPU driver.");
                    return nullptr;
                }
                gdevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL,
                    debugvideo, "direct3d12");
                deviceinfo.backend = RendererBackend::DX12;
                break;
            
            default:
                CE_LOG(LogLevel::Fatal, "[SDL_GPU Device Creator] Got invalid RendererBackend");
                return nullptr;
                break;
        }

        CE_LOG(LogLevel::Info,
            "[SDL_GPU Renderer] Backend given was: {}",
            static_cast<int>(deviceinfo.backend));
        
        if (gdevice == nullptr) {
            CE_LOG(LogLevel::Fatal,
                "[SDL_GPU Device Creator] SDL_CreateGPUDevice failed for backend {}: {}",
                static_cast<int>(deviceinfo.backend),
                SDL_GetError());
            return nullptr;
        }

        deviceinfo.device = gdevice;
        return std::make_shared<Renderer::GPUDevice>(deviceinfo);
    }

    void DestroyGPUDevice(GPUDeviceHandle device) {
        if (device == nullptr || device->device == nullptr) {
            return;
        }
        SDL_WaitForGPUIdle(static_cast<SDL_GPUDevice*>(device->device));
        SDL_DestroyGPUDevice(static_cast<SDL_GPUDevice*>(device->device));
    }
}
