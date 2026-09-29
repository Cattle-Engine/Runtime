#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>

#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    int SDLGPURenderer::BeginFrame([[maybe_unused]] SDL_Window* window) {
        mCommandBuffer = SDL_AcquireGPUCommandBuffer(mGPUDevice);
        if (mCommandBuffer == nullptr) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to acquire GPU command buffer: {}", SDL_GetError());
            return 1;
        }

        /*
            TODO:
            Have a file named something like 2d_rendering.cpp and have a function "Setup2DMVP" that does smth like this 
            gMVP = Utils::GetCameraMatrix(gCamera, (float)winW, (float)winH);
            (pulled from the old renderer)
        */

        mSwapchainTexture = nullptr;
        if(!SDL_WaitAndAcquireGPUSwapchainTexture(mCommandBuffer, mWindow, &mSwapchainTexture, nullptr, nullptr)) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire swapchain texture: {}", SDL_GetError());
            return 2;
        }

        if (mSwapchainTexture == nullptr) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Swapchain texture was nullptr!");
            return 3;
        }
    }
}