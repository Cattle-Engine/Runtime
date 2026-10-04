#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"

#include <cassert>
#include <SDL3/SDL.h>

#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderer.hpp"

#include <SDL3_image/SDL_image.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        constexpr SDL_GPUBufferCreateInfo kVertexBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * SDL_GPU_Renderer::kMaxVertices,
            .props = 0
        };

        constexpr SDL_GPUBufferCreateInfo kIndexBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(uint16_t) * SDL_GPU_Renderer::kMaxIndices,
            .props = 0
        };

        constexpr SDL_GPUTransferBufferCreateInfo kVertexUploadBufferCreateInfo {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vertex) * SDL_GPU_Renderer::kMaxVertices,
            .props = 0
        };

        constexpr SDL_GPUTransferBufferCreateInfo kIndexUploadBufferCreateInfo {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(uint16_t) * SDL_GPU_Renderer::kMaxIndices,
            .props = 0
        };
    }

    void SDL_GPU_Renderer::PreWinInit() {}

    int SDL_GPU_Renderer::Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) {
        (void)debug;
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

        mDevice = static_cast<SDL_GPUDevice*>(gdevice->device);

        if (mDevice == nullptr) {
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] gDevice is NULL!");
            return 2;
        }

        if (!SDL_ClaimWindowForGPUDevice(mDevice, window)) {
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Unable to bind window to GPU: {}", SDL_GetError());
            ShowError("[SDL_GPU Renderer] Unable to bind window to gpu device");
            return 3;
        }

        mWindowID = SDL_GetWindowID(window);

        const int pipelineResult = CreateDefaultPipeline(window);
        if (pipelineResult != 0) {
            return pipelineResult;
        }

        const int pipeline3DResult = CreateDefault3DPipeline(window);
        if (pipeline3DResult != 0) {
            return pipeline3DResult;
        }

        // Vertex buffer
        mVertexBuffer = SDL_CreateGPUBuffer(mDevice, &kVertexBufferCreateInfo);

        // Index buffer
        mIndexBuffer = SDL_CreateGPUBuffer(mDevice, &kIndexBufferCreateInfo);

        // Transfer buffers
        mTransferVerts = SDL_CreateGPUTransferBuffer(mDevice, &kVertexUploadBufferCreateInfo);
        mTransferIdx = SDL_CreateGPUTransferBuffer(mDevice, &kIndexUploadBufferCreateInfo);

        // Textured batch buffers
        mTexVertexBuffer = SDL_CreateGPUBuffer(mDevice, &kVertexBufferCreateInfo);
        mTexIndexBuffer = SDL_CreateGPUBuffer(mDevice, &kIndexBufferCreateInfo);
        mTransferTexVerts = SDL_CreateGPUTransferBuffer(mDevice, &kVertexUploadBufferCreateInfo);
        mTransferTexIdx = SDL_CreateGPUTransferBuffer(mDevice, &kIndexUploadBufferCreateInfo);

        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Batch buffers created");

        uint8_t white[4] = {255, 255, 255, 255};

        SDL_GPUTextureCreateInfo wTexInfo{};
        wTexInfo.type = SDL_GPU_TEXTURETYPE_2D;
        wTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        wTexInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        wTexInfo.width = 1;
        wTexInfo.height = 1;
        wTexInfo.layer_count_or_depth = 1;
        wTexInfo.num_levels = 1;
        mWhiteTex = SDL_CreateGPUTexture(mDevice, &wTexInfo);

        uint8_t errorTex[8 * 8 * 4];

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                bool checker = ((x + y) % 2) == 0;

                uint8_t* px = &errorTex[((y * 8) + x) * 4];

                if (checker) {
                    px[0] = 255; // R
                    px[1] = 0;   // G
                    px[2] = 255; // B
                    px[3] = 255; // A
                } else {
                    px[0] = 0;
                    px[1] = 0;
                    px[2] = 0;
                    px[3] = 255;
                }
            }
        }

        SDL_GPUTextureCreateInfo eTexInfo{};
        eTexInfo.type = SDL_GPU_TEXTURETYPE_2D;
        eTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        eTexInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        eTexInfo.width = 8;
        eTexInfo.height = 8;
        eTexInfo.layer_count_or_depth = 1;
        eTexInfo.num_levels = 1;

        mErrorTex = SDL_CreateGPUTexture(mDevice, &eTexInfo);

        SDL_GPUTransferBufferCreateInfo eTbInfo{};
        eTbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        eTbInfo.size = sizeof(errorTex);

        SDL_GPUTransferBuffer* eTb = SDL_CreateGPUTransferBuffer(mDevice, &eTbInfo);
        void* eMapped = SDL_MapGPUTransferBuffer(mDevice, eTb, false);
        SDL_memcpy(eMapped, errorTex, sizeof(errorTex));
        SDL_UnmapGPUTransferBuffer(mDevice, eTb);

        SDL_GPUCommandBuffer* eCmd = SDL_AcquireGPUCommandBuffer(mDevice);
        SDL_GPUCopyPass* eCopy = SDL_BeginGPUCopyPass(eCmd);

        SDL_GPUTextureTransferInfo eSrc{};
        eSrc.transfer_buffer = eTb;
        eSrc.pixels_per_row = 8;
        eSrc.rows_per_layer = 8;

        SDL_GPUTextureRegion eDst{};
        eDst.texture = mErrorTex;
        eDst.w = 8;
        eDst.h = 8;
        eDst.d = 1;

        SDL_UploadToGPUTexture(eCopy, &eSrc, &eDst, false);
        SDL_EndGPUCopyPass(eCopy);
        SDL_SubmitGPUCommandBuffer(eCmd);
        SDL_ReleaseGPUTransferBuffer(mDevice, eTb);

        SDL_GPUSamplerCreateInfo eSampInfo{};
        eSampInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        eSampInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        eSampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        eSampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        eSampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

        mErrorSampler = SDL_CreateGPUSampler(mDevice, &eSampInfo);

        SDL_GPUTransferBufferCreateInfo wTbInfo{};
        wTbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        wTbInfo.size = 4;
        SDL_GPUTransferBuffer* wTb = SDL_CreateGPUTransferBuffer(mDevice, &wTbInfo);
        void* wMapped = SDL_MapGPUTransferBuffer(mDevice, wTb, false);
        SDL_memcpy(wMapped, white, 4);
        SDL_UnmapGPUTransferBuffer(mDevice, wTb);

        SDL_GPUCommandBuffer* wCmd = SDL_AcquireGPUCommandBuffer(mDevice);
        SDL_GPUCopyPass* wCopy = SDL_BeginGPUCopyPass(wCmd);

        SDL_GPUTextureTransferInfo wSrc{};
        wSrc.transfer_buffer = wTb;
        wSrc.offset = 0;
        wSrc.pixels_per_row = 1;
        wSrc.rows_per_layer = 1;

        SDL_GPUTextureRegion wDst{};
        wDst.texture = mWhiteTex;
        wDst.w = 1;
        wDst.h = 1;
        wDst.d = 1;

        SDL_UploadToGPUTexture(wCopy, &wSrc, &wDst, false);
        SDL_EndGPUCopyPass(wCopy);
        SDL_SubmitGPUCommandBuffer(wCmd);
        SDL_ReleaseGPUTransferBuffer(mDevice, wTb);

        SDL_GPUSamplerCreateInfo wSampInfo{};
        wSampInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        wSampInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        wSampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        wSampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        wSampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        mWhiteSampler = SDL_CreateGPUSampler(mDevice, &wSampInfo);

        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] White fallback texture created");

        // Default normal
        uint8_t defaultNormal[4] = {128, 128, 255, 255};

        SDL_GPUTextureCreateInfo nTexInfo{};
        nTexInfo.type = SDL_GPU_TEXTURETYPE_2D;
        nTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        nTexInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        nTexInfo.width = 1;
        nTexInfo.height = 1;
        nTexInfo.layer_count_or_depth = 1;
        nTexInfo.num_levels = 1;
        mDefaultNormalTex = SDL_CreateGPUTexture(mDevice, &nTexInfo);

        SDL_GPUTransferBufferCreateInfo nTbInfo{};
        nTbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        nTbInfo.size = 4;
        SDL_GPUTransferBuffer* nTb = SDL_CreateGPUTransferBuffer(mDevice, &nTbInfo);
        void* nMapped = SDL_MapGPUTransferBuffer(mDevice, nTb, false);
        SDL_memcpy(nMapped, defaultNormal, 4);
        SDL_UnmapGPUTransferBuffer(mDevice, nTb);

        SDL_GPUCommandBuffer* nCmd = SDL_AcquireGPUCommandBuffer(mDevice);
        SDL_GPUCopyPass* nCopy = SDL_BeginGPUCopyPass(nCmd);

        SDL_GPUTextureTransferInfo nSrc{};
        nSrc.transfer_buffer = nTb;
        nSrc.offset = 0;
        nSrc.pixels_per_row = 1;
        nSrc.rows_per_layer = 1;

        SDL_GPUTextureRegion nDst{};
        nDst.texture = mDefaultNormalTex;
        nDst.w = 1;
        nDst.h = 1;
        nDst.d = 1;

        SDL_UploadToGPUTexture(nCopy, &nSrc, &nDst, false);
        SDL_EndGPUCopyPass(nCopy);
        SDL_SubmitGPUCommandBuffer(nCmd);
        SDL_ReleaseGPUTransferBuffer(mDevice, nTb);

        SDL_GPUSamplerCreateInfo nSampInfo{};
        nSampInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        nSampInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        nSampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        nSampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        nSampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        mNormalSampler = SDL_CreateGPUSampler(mDevice, &nSampInfo);

        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Default normal map texture created");

        ImGuiInit(window, mDevice);
        return 0;
    }

    int SDL_GPU_Renderer::Shutdown(SDL_Window* window) {
        CE_LOG(LogLevel::Info, "[Renderer {}] Shutdown called", static_cast<void*>(this));

        if (mDevice == nullptr) {
            CE_LOG(LogLevel::Error, "[Renderer {}] No device!", static_cast<void*>(this));
            return 0;
        }

        if (mDevice) {
            SDL_WaitForGPUIdle(mDevice);
        }

        SDL_ReleaseWindowFromGPUDevice(mDevice, window);
        if (mVertexBuffer)
            SDL_ReleaseGPUBuffer(mDevice, mVertexBuffer);
        if (mIndexBuffer)
            SDL_ReleaseGPUBuffer(mDevice, mIndexBuffer);
        if (mTexVertexBuffer)
            SDL_ReleaseGPUBuffer(mDevice, mTexVertexBuffer);
        if (mTexIndexBuffer)
            SDL_ReleaseGPUBuffer(mDevice, mTexIndexBuffer);

        if (mTransferVerts)
            SDL_ReleaseGPUTransferBuffer(mDevice, mTransferVerts);
        if (mTransferIdx)
            SDL_ReleaseGPUTransferBuffer(mDevice, mTransferIdx);
        if (mTransferTexVerts)
            SDL_ReleaseGPUTransferBuffer(mDevice, mTransferTexVerts);
        if (mTransferTexIdx)
            SDL_ReleaseGPUTransferBuffer(mDevice, mTransferTexIdx);

        DestroyDefaultPipeline();
        DestroyDefault3DPipeline();

        if (mWhiteSampler) {
            SDL_ReleaseGPUSampler(mDevice, mWhiteSampler);
            mWhiteSampler = nullptr;
        }

        CE_LOG(LogLevel::Info, "[Renderer {}] Destroying white texture at {}", static_cast<void*>(this),
               static_cast<void*>(mWhiteTex));
        if (mWhiteTex) {
            SDL_ReleaseGPUTexture(mDevice, mWhiteTex);
            mWhiteTex = nullptr;
        }

        if (mDefaultNormalTex) {
            SDL_ReleaseGPUTexture(mDevice, mDefaultNormalTex);
            mDefaultNormalTex = nullptr;
        }
        if (mNormalSampler) {
            SDL_ReleaseGPUSampler(mDevice, mNormalSampler);
            mNormalSampler = nullptr;
        }

        if (mErrorSampler) {
            SDL_ReleaseGPUSampler(mDevice, mErrorSampler);
            mErrorSampler = nullptr;
        }

        if (mErrorTex) {
            SDL_ReleaseGPUTexture(mDevice, mErrorTex);
            mErrorTex = nullptr;
        }

        ProcessDeferredDeletions(true);
        ImGuiShutdown();

        return 0;
    }
} // namespace CE::Renderer::SDL_GPU_Renderer
