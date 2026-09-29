#include <SDL3/SDL_gpu.h>
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    int SDLGPURenderer::Init(SDL_Window* window, [[maybe_unused]] bool debug, GPUDeviceHandle gdevice) {
        switch (gdevice->backend) {
        case RendererBackend::DX12:
        case RendererBackend::Metal:
        case RendererBackend::Vulkan:
            break;

        default:
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Unsupported GPU device was given!");
            CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Backend given was: {}", static_cast<int>(gdevice->backend));
            return 1;
        }

        mGPUDevice = static_cast<SDL_GPUDevice*>(gdevice->device);

        if (mGPUDevice == nullptr) {
            CE_LOG(LogLevel::Fatal, "[SDLGPURenderer] mGPUDevice is nullptr!");
            return 2;
        }

        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Claiming window for GPU device");
        if (!SDL_ClaimWindowForGPUDevice(mGPUDevice, window)) {
            CE_LOG(LogLevel::Fatal, "[SDLGPURenderer] Failed to claime window for GPU device");
            return 3;
        }

        const int pipeline_2d_result = Bootstrap_CreateDefault2DPipeline();
        if (pipeline_2d_result != 0) {
            return 4;
        }
    }
}