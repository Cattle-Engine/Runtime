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

        int win_w = 0;
        int win_h = 0;
        SDL_GetWindowSizeInPixels(mWindow, &win_w, &win_h);
        pRenderSize = glm::vec2(static_cast<float>(win_w), static_cast<float>(win_h));

        mSwapchainTexture = nullptr;
        if(!SDL_WaitAndAcquireGPUSwapchainTexture(mCommandBuffer, mWindow, &mSwapchainTexture, nullptr, nullptr)) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire swapchain texture: {}", SDL_GetError());
            return 2;
        }

        if (mSwapchainTexture == nullptr) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Swapchain texture was nullptr!");
            return 3;
        }

        mMappedIndices = nullptr;
        mMappedVertices = nullptr;
        mIndexCount = 0;
        mVertexCount = 0;

        mMappedVertices = static_cast<detail::Vertex*>(SDL_MapGPUTransferBuffer(mGPUDevice, mVertexUploadBuffer.Get(), true));
        mMappedIndices = static_cast<uint16_t*>(SDL_MapGPUTransferBuffer(mGPUDevice, mIndexUploadBuffer.Get(), true));
        return 0;
    }

    int SDLGPURenderer::EndFrame([[maybe_unused]] SDL_Window* window) {
        if (mMappedVertices != nullptr) {
            SDL_UnmapGPUTransferBuffer(mGPUDevice, mVertexUploadBuffer.Get());
            mMappedVertices = nullptr;
        }

        if (mMappedIndices != nullptr) {
            SDL_UnmapGPUTransferBuffer(mGPUDevice, mIndexUploadBuffer.Get());
            mMappedIndices = nullptr;
        }

        if (mVertexCount > 0 && mIndexCount > 0) {
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(mCommandBuffer);
            if (copy_pass != nullptr) {
                SDL_GPUTransferBufferLocation vertex_source{};
                vertex_source.transfer_buffer = mVertexUploadBuffer.Get();
                vertex_source.offset = 0;

                SDL_GPUBufferRegion vertex_destination{};
                vertex_destination.buffer = mVertexBuffer.Get();
                vertex_destination.offset = 0;
                vertex_destination.size = static_cast<Uint32>(mVertexCount * sizeof(detail::Vertex));

                SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, true);

                SDL_GPUTransferBufferLocation index_source{};
                index_source.transfer_buffer = mIndexUploadBuffer.Get();
                index_source.offset = 0;

                SDL_GPUBufferRegion index_destination{};
                index_destination.buffer = mIndexBuffer.Get();
                index_destination.offset = 0;
                index_destination.size = static_cast<Uint32>(mIndexCount * sizeof(uint16_t));

                SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, true);
                SDL_EndGPUCopyPass(copy_pass);
            }
        }

        SDL_GPUColorTargetInfo color_target{};
        color_target.texture = mSwapchainTexture;
        color_target.clear_color = {0.12f, 0.12f, 0.14f, 1.0f};
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(mCommandBuffer, &color_target, 1, nullptr);
        if (render_pass != nullptr) {
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(mCommandBuffer);

        mCommandBuffer = nullptr;
        mSwapchainTexture = nullptr;
        mVertexCount = 0;
        mIndexCount = 0;
        return 0;
    }
}