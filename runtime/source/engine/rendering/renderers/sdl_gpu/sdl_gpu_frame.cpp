
#include "imgui_impl_sdlgpu3.h"

#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    int SDL_GPU_Renderer::BeginFrame(SDL_Window* window) {
        mWarnedOutsideFrame = false;
        mCommandBuffer = SDL_AcquireGPUCommandBuffer(mDevice);
        if (mCommandBuffer == nullptr) {
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to acquire command buffer");
            return 1;
        }

        mMVP = Utils::GetCameraMatrix(mCamera2D, pRenderSize.x, pRenderSize.y);

        mSwapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(mCommandBuffer, window, &mSwapchainTexture, NULL, NULL)) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire swapchain texture: {}", SDL_GetError());
            return 2;
        }

        if (mSwapchainTexture == nullptr) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Swapchain texture was nullptr!");
            return 3;
        }
        mVertCount = 0;
        mIndexCount = 0;

        mTexVertCount = 0;
        mTexIndexCount = 0;

        mPrimitiveBatches.clear();
        mTexBatches.clear();
        mMeshCommands.clear();
        mCurrentPrimitiveShader = nullptr;
        mCurrentTex = nullptr;
        mCurrentTexSampler = nullptr;
        m3DModeActive = false;
        m2DModeActive = false;

        mMappedVerts = (Vertex*)SDL_MapGPUTransferBuffer(mDevice, mTransferVerts, true);
        mMappedIndices = (uint16_t*)SDL_MapGPUTransferBuffer(mDevice, mTransferIdx, true);

        mMappedTexVerts = (Vertex*)SDL_MapGPUTransferBuffer(mDevice, mTransferTexVerts, true);
        mMappedTexIndices = (uint16_t*)SDL_MapGPUTransferBuffer(mDevice, mTransferTexIdx, true);
        mFrameActive = true;
        return 0;
    }

    int SDL_GPU_Renderer::EndFrame([[maybe_unused]] SDL_Window* window) {
        SDL_UnmapGPUTransferBuffer(mDevice, mTransferVerts);
        SDL_UnmapGPUTransferBuffer(mDevice, mTransferIdx);
        SDL_UnmapGPUTransferBuffer(mDevice, mTransferTexVerts);
        SDL_UnmapGPUTransferBuffer(mDevice, mTransferTexIdx);

        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(mCommandBuffer);

        if (mIndexCount > 0) {
            size_t vSize = sizeof(Vertex) * mVertCount;
            size_t iSize = sizeof(uint16_t) * mIndexCount;

            SDL_GPUTransferBufferLocation vLoc{mTransferVerts, 0};
            SDL_GPUBufferRegion vReg{mVertexBuffer, 0, (Uint32)vSize};
            SDL_UploadToGPUBuffer(copy, &vLoc, &vReg, true);

            SDL_GPUTransferBufferLocation iLoc{mTransferIdx, 0};
            SDL_GPUBufferRegion iReg{mIndexBuffer, 0, (Uint32)iSize};
            SDL_UploadToGPUBuffer(copy, &iLoc, &iReg, true);
        }

        if (mTexIndexCount > 0) {
            size_t vSize = sizeof(Vertex) * mTexVertCount;
            size_t iSize = sizeof(uint16_t) * mTexIndexCount;

            SDL_GPUTransferBufferLocation vLoc{mTransferTexVerts, 0};
            SDL_GPUBufferRegion vReg{mTexVertexBuffer, 0, (Uint32)vSize};
            SDL_UploadToGPUBuffer(copy, &vLoc, &vReg, true);

            SDL_GPUTransferBufferLocation iLoc{mTransferTexIdx, 0};
            SDL_GPUBufferRegion iReg{mTexIndexBuffer, 0, (Uint32)iSize};
            SDL_UploadToGPUBuffer(copy, &iLoc, &iReg, true);
        }

        SDL_EndGPUCopyPass(copy);

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.texture = mRenderTexture.Get();
        colorTargetInfo.clear_color = mClearColor;
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        const CubeMap& skyboxState = GetSkyBoxState();
        const bool hasSkybox =
            skyboxState.front ||
            skyboxState.back ||
            skyboxState.left ||
            skyboxState.right ||
            skyboxState.top ||
            skyboxState.bottom;

        if (!mMeshCommands.empty() || hasSkybox) {
            DrawQueuedMeshes();
            colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
        }

        mRenderPass = SDL_BeginGPURenderPass(
            mCommandBuffer,
            &colorTargetInfo,
            1,
            nullptr
        );

        BindActivePipeline();
        PushActiveShaderUniforms();
        BindShaderSamplers(mWhiteTex, mWhiteSampler);

        if (mIndexCount > 0) {
            SDL_GPUBufferBinding vBind{mVertexBuffer, 0};
            SDL_GPUBufferBinding iBind{mIndexBuffer, 0};

            SDL_BindGPUVertexBuffers(
                mRenderPass,
                0,
                &vBind,
                1
            );

            SDL_BindGPUIndexBuffer(
                mRenderPass,
                &iBind,
                SDL_GPU_INDEXELEMENTSIZE_16BIT
            );

            for (const auto& batch : mPrimitiveBatches) {
                if (batch.idxCount == 0) {
                    continue;
                }

                mCurrentShader = batch.shader;

                BindActivePipeline();
                PushActiveShaderUniforms();
                BindShaderSamplers(mWhiteTex, mWhiteSampler);

                SDL_DrawGPUIndexedPrimitives(
                    mRenderPass,
                    batch.idxCount,
                    1,
                    batch.idxOffset,
                    0,
                    0
                );
            }
        }

        for (const auto& batch : mTexBatches) {
            if (batch.idxCount == 0) {
                continue;
            }

            if (!batch.texture || !batch.texture->gpuTex) {
                continue;
            }

            mCurrentShader = batch.shader;

            BindActivePipeline();
            PushActiveShaderUniforms();
            BindShaderSamplers(
                batch.texture->gpuTex,
                batch.sampler
            );

            SDL_GPUBufferBinding vBind{mTexVertexBuffer, 0};
            SDL_GPUBufferBinding iBind{mTexIndexBuffer, 0};

            SDL_BindGPUVertexBuffers(
                mRenderPass,
                0,
                &vBind,
                1
            );

            SDL_BindGPUIndexBuffer(
                mRenderPass,
                &iBind,
                SDL_GPU_INDEXELEMENTSIZE_16BIT
            );

            SDL_DrawGPUIndexedPrimitives(
                mRenderPass,
                batch.idxCount,
                1,
                batch.idxOffset,
                0,
                0
            );
        }

        mCurrentShader = nullptr;

        SDL_EndGPURenderPass(mRenderPass);
        mRenderPass = nullptr;

        SDL_GPUColorTargetInfo upscaleTarget{};
        upscaleTarget.texture = mSwapchainTexture;
        upscaleTarget.clear_color = mClearColor;
        upscaleTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        upscaleTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* upscalePass = SDL_BeginGPURenderPass(
            mCommandBuffer,
            &upscaleTarget,
            1,
            nullptr
        );

        SDL_BindGPUGraphicsPipeline(upscalePass, mUpscalePipeline);
        const SDL_GPUTextureSamplerBinding renderTextureBinding{
            .texture = mRenderTexture.Get(),
            .sampler = mWhiteSampler,
        };
        SDL_BindGPUFragmentSamplers(upscalePass, 0, &renderTextureBinding, 1);
        SDL_DrawGPUPrimitives(upscalePass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(upscalePass);

        if (mPendingImGuiDrawData && mSwapchainTexture) {
            ImGui::SetCurrentContext(mImguicontext);

            ImGui_ImplSDLGPU3_PrepareDrawData(
                mPendingImGuiDrawData,
                mCommandBuffer
            );

            SDL_GPUColorTargetInfo uiTarget{};
            uiTarget.texture = mSwapchainTexture;
            uiTarget.load_op = SDL_GPU_LOADOP_LOAD;
            uiTarget.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* uiPass = SDL_BeginGPURenderPass(
                mCommandBuffer,
                &uiTarget,
                1,
                nullptr
            );

            ImGui_ImplSDLGPU3_RenderDrawData(
                mPendingImGuiDrawData,
                mCommandBuffer,
                uiPass
            );

            SDL_EndGPURenderPass(uiPass);
        }

        SDL_SubmitGPUCommandBuffer(mCommandBuffer);

        mCommandBuffer = nullptr;
        mPendingImGuiDrawData = nullptr;
        mSwapchainTexture = nullptr;

        mMappedVerts = nullptr;
        mMappedIndices = nullptr;
        mMappedTexVerts = nullptr;
        mMappedTexIndices = nullptr;

        mFrameActive = false;

        ProcessDeferredDeletions();

        return 0;
    }
}
