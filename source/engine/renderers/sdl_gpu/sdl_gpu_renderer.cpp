#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <limits>
#include <cassert>
#include <cmath>

#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/common/vfs.hpp"
#include "imgui_impl_sdlgpu3.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CE::Renderer::SDL_GPU_Renderer {
    void RotatePoint(float& x, float& y, float cx, float cy, float sinA, float cosA) {
        float dx = x - cx;
        float dy = y - cy;
        x = cx + dx * cosA - dy * sinA;
        y = cy + dx * sinA + dy * cosA;
    }
    
    void SDL_GPU_Renderer::PreWinInit() {
        return;
    }

    int SDL_GPU_Renderer::Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) {
        switch (gdevice->backend)
        {
            case RendererBackend::DX12:
            case RendererBackend::Metal:
            case RendererBackend::Vulkan:
                break;

            default:
                CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Unsupported GPU device was given!");
                CE::Log(LogLevel::Info,
                    "[SDL_GPU Renderer] Backend given was: {}",
                    static_cast<int>(gdevice->backend));
                return 1;
        }

        gDevice = static_cast<SDL_GPUDevice*>(gdevice->device);

        if (gDevice == nullptr) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] gDevice is NULL!");
            return 2;
        }

        if (!SDL_ClaimWindowForGPUDevice(gDevice, window)) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Unable to bind window to GPU: {}", SDL_GetError());
            ShowError("[SDL_GPU Renderer] Unable to bind window to gpu device");
            return 3;
        }

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Loading vertex shader");
        SDL_GPUShader* vertexShader = Utils::LoadShader(gDevice, "standard_vertex.vert", 0, 1, 0, 0, gVFS);
        if (vertexShader == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create vertex shader!");
            return 4;
        }

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Loading fragment shader");
        SDL_GPUShader* fragmentShader = Utils::LoadShader(gDevice, "standard_fragment.frag", 1, 0, 0, 0, gVFS);
        if (fragmentShader == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create fragment shader!");
            return 5;
        }

        // Colour target
        SDL_GPUColorTargetDescription colorDesc{};
        colorDesc.format = SDL_GetGPUSwapchainTextureFormat(gDevice, window);

        // Vertex buffer layout
        SDL_GPUVertexBufferDescription vbDesc{};
        vbDesc.slot               = 0;
        vbDesc.input_rate         = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vbDesc.instance_step_rate = 0;
        vbDesc.pitch              = sizeof(Vertex);

        // Vertex attributes
        SDL_GPUVertexAttribute attrs[3]{};

        attrs[0].buffer_slot = 0;
        attrs[0].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].location    = 0;
        attrs[0].offset      = 0;

        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].location    = 1;
        attrs[1].offset      = sizeof(float) * 3;

        attrs[2].buffer_slot = 0;
        attrs[2].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].location    = 2;
        attrs[2].offset      = sizeof(float) * 3 + sizeof(uint8_t) * 4;

        // Pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.target_info.num_color_targets         = 1;
        pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;

        pipelineCreateInfo.vertex_input_state.num_vertex_buffers         = 1;
        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes      = 3;
        pipelineCreateInfo.vertex_input_state.vertex_attributes          = attrs;

        pipelineCreateInfo.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.vertex_shader   = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;

        SDL_GPUColorTargetBlendState blend{};
        blend.enable_blend = true;

        blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.color_blend_op = SDL_GPU_BLENDOP_ADD;

        blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

        blend.color_write_mask =
            SDL_GPU_COLORCOMPONENT_R |
            SDL_GPU_COLORCOMPONENT_G |
            SDL_GPU_COLORCOMPONENT_B |
            SDL_GPU_COLORCOMPONENT_A;

        colorDesc.blend_state = blend;

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Creating graphics pipeline");
        gPipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineCreateInfo);
        if (!gPipeline) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create pipeline: {}", SDL_GetError());
            return 6;
        }

        SDL_ReleaseGPUShader(gDevice, vertexShader);
        SDL_ReleaseGPUShader(gDevice, fragmentShader);
        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Created graphics pipeline");

        // Vertex buffer
        SDL_GPUBufferCreateInfo vbInfo{};
        vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbInfo.size  = sizeof(Vertex) * MAX_VERTS;
        gVertexBuffer = SDL_CreateGPUBuffer(gDevice, &vbInfo);

        // Index buffer
        SDL_GPUBufferCreateInfo ibInfo{};
        ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibInfo.size  = sizeof(uint16_t) * MAX_INDICES;
        gIndexBuffer = SDL_CreateGPUBuffer(gDevice, &ibInfo);

        // Transfer buffers
        SDL_GPUTransferBufferCreateInfo tvInfo{};
        tvInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tvInfo.size  = sizeof(Vertex) * MAX_VERTS;
        gTransferVerts = SDL_CreateGPUTransferBuffer(gDevice, &tvInfo);

        SDL_GPUTransferBufferCreateInfo tiInfo{};
        tiInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tiInfo.size  = sizeof(uint16_t) * MAX_INDICES;
        gTransferIdx = SDL_CreateGPUTransferBuffer(gDevice, &tiInfo);

        // Textured batch buffers
        SDL_GPUBufferCreateInfo tvbInfo{};
        tvbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        tvbInfo.size  = sizeof(Vertex) * MAX_VERTS;
        gTexVertexBuffer = SDL_CreateGPUBuffer(gDevice, &tvbInfo);

        SDL_GPUBufferCreateInfo tibInfo{};
        tibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        tibInfo.size  = sizeof(uint16_t) * MAX_INDICES;
        gTexIndexBuffer = SDL_CreateGPUBuffer(gDevice, &tibInfo);

        SDL_GPUTransferBufferCreateInfo ttvInfo{};
        ttvInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        ttvInfo.size  = sizeof(Vertex) * MAX_VERTS;
        gTransferTexVerts = SDL_CreateGPUTransferBuffer(gDevice, &ttvInfo);

        SDL_GPUTransferBufferCreateInfo ttiInfo{};
        ttiInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        ttiInfo.size  = sizeof(uint16_t) * MAX_INDICES;
        gTransferTexIdx = SDL_CreateGPUTransferBuffer(gDevice, &ttiInfo);

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Batch buffers created");

        uint8_t white[4] = { 255, 255, 255, 255 };

        SDL_GPUTextureCreateInfo wTexInfo{};
        wTexInfo.type                 = SDL_GPU_TEXTURETYPE_2D;
        wTexInfo.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        wTexInfo.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        wTexInfo.width                = 1;
        wTexInfo.height               = 1;
        wTexInfo.layer_count_or_depth = 1;
        wTexInfo.num_levels           = 1;
        gWhiteTex = SDL_CreateGPUTexture(gDevice, &wTexInfo);

        uint8_t errorTex[8 * 8 * 4];

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                bool checker = ((x + y) % 2) == 0;

                uint8_t* px = &errorTex[(y * 8 + x) * 4];

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
        eTexInfo.type   = SDL_GPU_TEXTURETYPE_2D;
        eTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        eTexInfo.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        eTexInfo.width  = 8;
        eTexInfo.height = 8;
        eTexInfo.layer_count_or_depth = 1;
        eTexInfo.num_levels = 1;

        gErrorTex = SDL_CreateGPUTexture(gDevice, &eTexInfo);

        SDL_GPUTransferBufferCreateInfo eTbInfo{};
        eTbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        eTbInfo.size  = sizeof(errorTex);

        SDL_GPUTransferBuffer* eTb = SDL_CreateGPUTransferBuffer(gDevice, &eTbInfo);
        void* eMapped = SDL_MapGPUTransferBuffer(gDevice, eTb, false);
        SDL_memcpy(eMapped, errorTex, sizeof(errorTex));
        SDL_UnmapGPUTransferBuffer(gDevice, eTb);

        SDL_GPUCommandBuffer* eCmd  = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass*      eCopy = SDL_BeginGPUCopyPass(eCmd);

        SDL_GPUTextureTransferInfo eSrc{};
        eSrc.transfer_buffer = eTb;
        eSrc.pixels_per_row  = 8;
        eSrc.rows_per_layer  = 8;

        SDL_GPUTextureRegion eDst{};
        eDst.texture = gErrorTex;
        eDst.w = 8;
        eDst.h = 8;
        eDst.d = 1;

        SDL_UploadToGPUTexture(eCopy, &eSrc, &eDst, false);
        SDL_EndGPUCopyPass(eCopy);
        SDL_SubmitGPUCommandBuffer(eCmd);
        SDL_ReleaseGPUTransferBuffer(gDevice, eTb);
    
        SDL_GPUSamplerCreateInfo eSampInfo{};
        eSampInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        eSampInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        eSampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        eSampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        eSampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

        gErrorSampler = SDL_CreateGPUSampler(gDevice, &eSampInfo);

        SDL_GPUTransferBufferCreateInfo wTbInfo{};
        wTbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        wTbInfo.size  = 4;
        SDL_GPUTransferBuffer* wTb     = SDL_CreateGPUTransferBuffer(gDevice, &wTbInfo);
        void*                  wMapped = SDL_MapGPUTransferBuffer(gDevice, wTb, false);
        SDL_memcpy(wMapped, white, 4);
        SDL_UnmapGPUTransferBuffer(gDevice, wTb);

        SDL_GPUCommandBuffer* wCmd  = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass*      wCopy = SDL_BeginGPUCopyPass(wCmd);

        SDL_GPUTextureTransferInfo wSrc{};
        wSrc.transfer_buffer = wTb;
        wSrc.offset          = 0;
        wSrc.pixels_per_row  = 1;
        wSrc.rows_per_layer  = 1;

        SDL_GPUTextureRegion wDst{};
        wDst.texture = gWhiteTex;
        wDst.w       = 1;
        wDst.h       = 1;
        wDst.d       = 1;

        SDL_UploadToGPUTexture(wCopy, &wSrc, &wDst, false);
        SDL_EndGPUCopyPass(wCopy);
        SDL_SubmitGPUCommandBuffer(wCmd);
        SDL_ReleaseGPUTransferBuffer(gDevice, wTb);

        SDL_GPUSamplerCreateInfo wSampInfo{};
        wSampInfo.min_filter     = SDL_GPU_FILTER_NEAREST;
        wSampInfo.mag_filter     = SDL_GPU_FILTER_NEAREST;
        wSampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        wSampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        wSampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        gWhiteSampler = SDL_CreateGPUSampler(gDevice, &wSampInfo);

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] White fallback texture created");
        ImGuiInit(window, gDevice);
        return 0;
    }

    int SDL_GPU_Renderer::Shutdown(SDL_Window* window) {
        CE::Log(LogLevel::Info, "[Renderer {}] Shutdown called", static_cast<void*>(this));

        if(gDevice == nullptr) {
            CE::Log(LogLevel::Error, "[Renderer {}] No device!", static_cast<void*>(this));
            return 0;
        }
        
        if (gDevice) {
            SDL_WaitForGPUIdle(gDevice);
        }

        SDL_ReleaseWindowFromGPUDevice(gDevice, window);
        if (gVertexBuffer)      SDL_ReleaseGPUBuffer(gDevice, gVertexBuffer);
        if (gIndexBuffer)       SDL_ReleaseGPUBuffer(gDevice, gIndexBuffer);
        if (gTexVertexBuffer)   SDL_ReleaseGPUBuffer(gDevice, gTexVertexBuffer);
        if (gTexIndexBuffer)    SDL_ReleaseGPUBuffer(gDevice, gTexIndexBuffer);

        if (gTransferVerts)     SDL_ReleaseGPUTransferBuffer(gDevice, gTransferVerts);
        if (gTransferIdx)       SDL_ReleaseGPUTransferBuffer(gDevice, gTransferIdx);
        if (gTransferTexVerts)  SDL_ReleaseGPUTransferBuffer(gDevice, gTransferTexVerts);
        if (gTransferTexIdx)    SDL_ReleaseGPUTransferBuffer(gDevice, gTransferTexIdx);

        if (gPipeline)          SDL_ReleaseGPUGraphicsPipeline(gDevice, gPipeline);

        if (gWhiteSampler) {
            SDL_ReleaseGPUSampler(gDevice, gWhiteSampler);
            gWhiteSampler = nullptr;
        }  
        
        CE::Log(LogLevel::Info, "[Renderer {}] Destroying white texture at {}", 
                static_cast<void*>(this), static_cast<void*>(gWhiteTex));
        if (gWhiteTex) {
            SDL_ReleaseGPUTexture(gDevice, gWhiteTex);
            gWhiteTex = nullptr;
        }

        if (gErrorSampler) {
            SDL_ReleaseGPUSampler(gDevice, gErrorSampler);
            gErrorSampler = nullptr;
        }

        if (gErrorTex) {
            SDL_ReleaseGPUTexture(gDevice, gErrorTex);
            gErrorTex = nullptr;
        }

        ImGuiShutdown();

        return 0;
    }
    int SDL_GPU_Renderer::BeginFrame(SDL_Window* window) {
        gCommandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        if (gCommandBuffer == nullptr) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to acquire command buffer");
            return 1;
        }

        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);
        gMVP = Utils::GetCameraMatrix(gCamera, (float)winW, (float)winH);

        gSwapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(gCommandBuffer, window, &gSwapchainTexture, NULL, NULL)) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire swapchain texture: {}", SDL_GetError());
            return 2;
        }

        if (gSwapchainTexture == nullptr) return 3;

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.texture     = gSwapchainTexture;
        colorTargetInfo.clear_color = gClearColor;
        colorTargetInfo.load_op     = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op    = SDL_GPU_STOREOP_STORE;

        gRenderPass = SDL_BeginGPURenderPass(gCommandBuffer, &colorTargetInfo, 1, NULL);

        SDL_BindGPUGraphicsPipeline(gRenderPass, gPipeline);
        SDL_PushGPUVertexUniformData(gCommandBuffer, 0, &gMVP, sizeof(glm::mat4));

        SDL_GPUTextureSamplerBinding defaultBinding{};
        defaultBinding.texture = gWhiteTex;
        defaultBinding.sampler = gWhiteSampler;
        SDL_BindGPUFragmentSamplers(gRenderPass, 0, &defaultBinding, 1);

        gVertCount  = 0;
        gIndexCount = 0;

        gTexVertCount  = 0;
        gTexIndexCount = 0;

        gTexBatches.clear();
        gCurrentTex = nullptr;

        gMappedVerts   = (Vertex*)SDL_MapGPUTransferBuffer(gDevice, gTransferVerts, true);
        gMappedIndices = (uint16_t*)SDL_MapGPUTransferBuffer(gDevice, gTransferIdx, true);

        gMappedTexVerts   = (Vertex*)SDL_MapGPUTransferBuffer(gDevice, gTransferTexVerts, true);
        gMappedTexIndices = (uint16_t*)SDL_MapGPUTransferBuffer(gDevice, gTransferTexIdx, true);
        return 0;
    }

    int SDL_GPU_Renderer::EndFrame(SDL_Window* window) {
        (void)window;

        SDL_UnmapGPUTransferBuffer(gDevice, gTransferVerts);
        SDL_UnmapGPUTransferBuffer(gDevice, gTransferIdx);
        SDL_UnmapGPUTransferBuffer(gDevice, gTransferTexVerts);
        SDL_UnmapGPUTransferBuffer(gDevice, gTransferTexIdx);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(uploadCmd);

        if (gIndexCount > 0) {
            size_t vSize = sizeof(Vertex) * gVertCount;
            size_t iSize = sizeof(uint16_t) * gIndexCount;

            SDL_GPUTransferBufferLocation vLoc{ gTransferVerts, 0 };
            SDL_GPUBufferRegion vReg{ gVertexBuffer, 0, (Uint32)vSize };
            SDL_UploadToGPUBuffer(copy, &vLoc, &vReg, true);

            SDL_GPUTransferBufferLocation iLoc{ gTransferIdx, 0 };
            SDL_GPUBufferRegion iReg{ gIndexBuffer, 0, (Uint32)iSize };
            SDL_UploadToGPUBuffer(copy, &iLoc, &iReg, true);
        }

        if (gTexIndexCount > 0) {
            size_t vSize = sizeof(Vertex) * gTexVertCount;
            size_t iSize = sizeof(uint16_t) * gTexIndexCount;

            SDL_GPUTransferBufferLocation vLoc{ gTransferTexVerts, 0 };
            SDL_GPUBufferRegion vReg{ gTexVertexBuffer, 0, (Uint32)vSize };
            SDL_UploadToGPUBuffer(copy, &vLoc, &vReg, true);

            SDL_GPUTransferBufferLocation iLoc{ gTransferTexIdx, 0 };
            SDL_GPUBufferRegion iReg{ gTexIndexBuffer, 0, (Uint32)iSize };
            SDL_UploadToGPUBuffer(copy, &iLoc, &iReg, true);
        }

        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        if (gIndexCount > 0) {
            SDL_GPUTextureSamplerBinding binding{ gWhiteTex, gWhiteSampler };
            SDL_BindGPUFragmentSamplers(gRenderPass, 0, &binding, 1);

            SDL_GPUBufferBinding vBind{ gVertexBuffer, 0 };
            SDL_GPUBufferBinding iBind{ gIndexBuffer,  0 };

            SDL_BindGPUVertexBuffers(gRenderPass, 0, &vBind, 1);
            SDL_BindGPUIndexBuffer(gRenderPass, &iBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            SDL_DrawGPUIndexedPrimitives(gRenderPass, gIndexCount, 1, 0, 0, 0);
        }

        for (const auto& batch : gTexBatches) {
            if (batch.idxCount == 0) continue;

            SDL_GPUTextureSamplerBinding binding{
                batch.texture->gpuTex,
                batch.texture->sampler
            };

            SDL_BindGPUFragmentSamplers(gRenderPass, 0, &binding, 1);

            SDL_GPUBufferBinding vBind{ gTexVertexBuffer, 0 };
            SDL_GPUBufferBinding iBind{ gTexIndexBuffer,  0 };

            SDL_BindGPUVertexBuffers(gRenderPass, 0, &vBind, 1);
            SDL_BindGPUIndexBuffer(gRenderPass, &iBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            SDL_DrawGPUIndexedPrimitives(
                gRenderPass,
                batch.idxCount,
                1,
                batch.idxOffset,
                0,
                0
            );
        }

        SDL_EndGPURenderPass(gRenderPass);

        if (mPendingImGuiDrawData && gSwapchainTexture) {
            ImGui::SetCurrentContext(mImguicontext);
            ImGui_ImplSDLGPU3_PrepareDrawData(mPendingImGuiDrawData, gCommandBuffer);

            SDL_GPUColorTargetInfo uiTarget{};
            uiTarget.texture  = gSwapchainTexture;
            uiTarget.load_op  = SDL_GPU_LOADOP_LOAD;
            uiTarget.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* uiPass = SDL_BeginGPURenderPass(gCommandBuffer, &uiTarget, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(mPendingImGuiDrawData, gCommandBuffer, uiPass);
            SDL_EndGPURenderPass(uiPass);
        }

        SDL_SubmitGPUCommandBuffer(gCommandBuffer);
        mPendingImGuiDrawData = nullptr;
        gSwapchainTexture = nullptr;
        return 0;
    }

    void SDL_GPU_Renderer::DrawRect(float x, float y, float w, float h,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                    float rotation) {
        if (gVertCount + 4 > MAX_VERTS || gIndexCount + 6 > MAX_INDICES) return;

        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[4] = { x,     x + w, x + w, x     };
        float py[4] = { y,     y,     y + h, y + h  };

        uint16_t base = (uint16_t)gVertCount;
        for (int i = 0; i < 4; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            gMappedVerts[base + i] = { px[i], py[i], 0, r, g, b, a, 0, 0 };
        }

        gMappedIndices[gIndexCount++] = base;
        gMappedIndices[gIndexCount++] = base + 1;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 3;
        gMappedIndices[gIndexCount++] = base;
        gVertCount += 4;
    }
    
    void SDL_GPU_Renderer::DrawRectLines(float x, float y, float w, float h,
                                        float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        DrawLine(x,     y,     x + w, y,     thickness, r, g, b, a); // top
        DrawLine(x + w, y,     x + w, y + h, thickness, r, g, b, a); // right
        DrawLine(x + w, y + h, x,     y + h, thickness, r, g, b, a); // bottom
        DrawLine(x,     y + h, x,     y,     thickness, r, g, b, a); // left
    }

    void SDL_GPU_Renderer::DrawTriangle(
        float x0, float y0,
        float x1, float y1,
        float x2, float y2,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        float rotation) {
        if (gVertCount + 3 > MAX_VERTS || gIndexCount + 3 > MAX_INDICES) return;

        // Centroid is the average of the three vertices
        float cx = (x0 + x1 + x2) / 3.0f;
        float cy = (y0 + y1 + y2) / 3.0f;
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[3] = { x0, x1, x2 };
        float py[3] = { y0, y1, y2 };

        uint16_t base = (uint16_t)gVertCount;
        for (int i = 0; i < 3; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            gMappedVerts[base + i] = { px[i], py[i], 0, r, g, b, a, 0, 0 };
        }

        gMappedIndices[gIndexCount++] = base;
        gMappedIndices[gIndexCount++] = base + 1;
        gMappedIndices[gIndexCount++] = base + 2;

        gVertCount += 3;
    }

    void SDL_GPU_Renderer::DrawLine(float x1, float y1, float x2, float y2,
                                float thickness,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (gVertCount + 4 > MAX_VERTS || gIndexCount + 6 > MAX_INDICES) {
            CE::Log(LogLevel::Warn, "[SDL_GPU Renderer] Batch full, skipping line");
            return;
        }

        float dx  = x2 - x1;
        float dy  = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6f) return;

        // Perpendicular unit vector scaled to half-thickness
        float nx = (-dy / len) * (thickness * 0.5f);
        float ny = ( dx / len) * (thickness * 0.5f);

        uint16_t base = (uint16_t)gVertCount;

        gMappedVerts[base + 0] = { x1 + nx, y1 + ny, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 1] = { x2 + nx, y2 + ny, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 2] = { x2 - nx, y2 - ny, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 3] = { x1 - nx, y1 - ny, 0, r, g, b, a, 0, 0 };

        gMappedIndices[gIndexCount++] = base;
        gMappedIndices[gIndexCount++] = base + 1;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 3;
        gMappedIndices[gIndexCount++] = base;

        gVertCount += 4;
    }

    void SDL_GPU_Renderer::DrawCircle(float cx, float cy, float radius,
                                    int segments,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (segments < 3) segments = 3;

        uint32_t vNeeded = (uint32_t)(segments + 1); // centre + rim
        uint32_t iNeeded = (uint32_t)(segments * 3);

        if (gVertCount + vNeeded > MAX_VERTS || gIndexCount + iNeeded > MAX_INDICES) {
            CE::Log(LogLevel::Warn, "[SDL_GPU Renderer] Batch full, skipping circle");
            return;
        }

        uint16_t centre = (uint16_t)gVertCount;
        gMappedVerts[centre] = { cx, cy, 0, r, g, b, a, 0.5f, 0.5f };
        gVertCount++;

        float step = (float)(2.0 * M_PI) / (float)segments;
        for (int i = 0; i < segments; ++i) {
            float angle = step * (float)i;
            float vx    = cx + std::cos(angle) * radius;
            float vy    = cy + std::sin(angle) * radius;
            float u     = (std::cos(angle) + 1.0f) * 0.5f;
            float v     = (std::sin(angle) + 1.0f) * 0.5f;
            gMappedVerts[gVertCount + i] = { vx, vy, 0, r, g, b, a, u, v };
        }

        for (int i = 0; i < segments; ++i) {
            uint16_t cur  = (uint16_t)(gVertCount + i);
            uint16_t next = (uint16_t)(gVertCount + (i + 1) % segments);
            gMappedIndices[gIndexCount++] = centre;
            gMappedIndices[gIndexCount++] = cur;
            gMappedIndices[gIndexCount++] = next;
        }

        gVertCount += (uint32_t)segments;
    }

    void SDL_GPU_Renderer::DrawCircleLines(float cx, float cy, float radius,
                                        int segments, float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (segments < 3) segments = 3;
        float step = (float)(2.0 * M_PI) / (float)segments;
        for (int i = 0; i < segments; ++i) {
            float a0 = step * (float)i;
            float a1 = step * (float)(i + 1);
            DrawLine(cx + std::cos(a0) * radius, cy + std::sin(a0) * radius,
                    cx + std::cos(a1) * radius, cy + std::sin(a1) * radius,
                    thickness, r, g, b, a);
        }
    }


    Texture* SDL_GPU_Renderer::LoadTex(const char* path) {
        CE::VFS::VFS& vfs = *gVFS;

        uint64_t sz = 0;
        if (!vfs.GetFileSize(path, sz) || sz == 0) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] VFS could not stat '{}' (missing or empty)", path);
            return nullptr;
        }

        VirtualFile* vf = vfs.OpenFile(path);
        if (!vf) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] VFS could not open '{}'", path);
            return nullptr;
        }

        std::vector<uint8_t> fileBytes((size_t)sz);
        vfs.ReadFile(vf, fileBytes.data(), fileBytes.size());
        vfs.CloseFile(vf);

        SDL_IOStream* mem = SDL_IOFromConstMem(fileBytes.data(), fileBytes.size());
        if (!mem) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] SDL_IOFromConstMem failed: {}", SDL_GetError());
            return nullptr;
        }

        SDL_Surface* surface = IMG_Load_IO(mem, true); 
        if (!surface) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] IMG_Load_IO failed for '{}': {}", path, SDL_GetError());
            return nullptr;
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] SDL_ConvertSurface failed: {}", SDL_GetError());
            return nullptr;
        }

        int    w        = converted->w;
        int    h        = converted->h;
        size_t dataSize = (size_t)(w * h * 4);

        SDL_GPUTextureCreateInfo texInfo{};
        texInfo.type                 = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width                = (Uint32)w;
        texInfo.height               = (Uint32)h;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels           = 1;

        SDL_GPUTexture* gpuTex = SDL_CreateGPUTexture(gDevice, &texInfo);
        if (!gpuTex) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] SDL_CreateGPUTexture failed: {}", SDL_GetError());
            SDL_DestroySurface(converted);
            return nullptr;
        }

        SDL_GPUTransferBufferCreateInfo tbInfo{};
        tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbInfo.size  = (Uint32)dataSize;

        SDL_GPUTransferBuffer* tb     = SDL_CreateGPUTransferBuffer(gDevice, &tbInfo);
        void*                  mapped = SDL_MapGPUTransferBuffer(gDevice, tb, false);
        SDL_memcpy(mapped, converted->pixels, dataSize);
        SDL_UnmapGPUTransferBuffer(gDevice, tb);
        SDL_DestroySurface(converted);

        SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass*      copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tb;
        src.offset          = 0;
        src.pixels_per_row  = (Uint32)w;
        src.rows_per_layer  = (Uint32)h;

        SDL_GPUTextureRegion dst{};
        dst.texture = gpuTex;
        dst.w       = (Uint32)w;
        dst.h       = (Uint32)h;
        dst.d       = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(gDevice, tb);

        // Sampler
        SDL_GPUSamplerCreateInfo sampInfo{};
        sampInfo.min_filter     = SDL_GPU_FILTER_LINEAR;
        sampInfo.mag_filter     = SDL_GPU_FILTER_LINEAR;
        sampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        SDLGPUTexData* data = new SDLGPUTexData();
        data->gpuTex        = gpuTex;
        data->sampler       = SDL_CreateGPUSampler(gDevice, &sampInfo);

        Texture* tex = new Texture();
        tex->handle  = data;
        tex->width   = w;
        tex->height  = h;
        tex->format  = TextureFormat::RGBA8;
        tex->backend = RendererBackend::Vulkan;
        return tex;
    }

    void SDL_GPU_Renderer::DrawTex(Texture* texture, float x, float y,
                                    float w, float h, Colour colour,
                                    float rotation) {
        if (!texture || !texture->handle) return;
        auto* tex = static_cast<SDLGPUTexData*>(texture->handle);
        if (gTexVertCount + 4 > MAX_VERTS || gTexIndexCount + 6 > MAX_INDICES) return;

        if (gCurrentTex != tex) {
            gTexBatches.push_back({ tex, gTexVertCount, 0, gTexIndexCount, 0 });
            gCurrentTex = tex;
        }

        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[4] = { x,     x + w, x + w, x     };
        float py[4] = { y,     y,     y + h, y + h  };
        float pu[4] = { 0, 1, 1, 0 };
        float pv[4] = { 0, 0, 1, 1 };

        uint16_t base = (uint16_t)gTexVertCount;
        uint8_t r = colour.r, g = colour.g, b = colour.b, a = colour.a;

        for (int i = 0; i < 4; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            gMappedTexVerts[base + i] = { px[i], py[i], 0, r, g, b, a, pu[i], pv[i] };
        }

        gMappedTexIndices[gTexIndexCount + 0] = base;
        gMappedTexIndices[gTexIndexCount + 1] = base + 1;
        gMappedTexIndices[gTexIndexCount + 2] = base + 2;
        gMappedTexIndices[gTexIndexCount + 3] = base + 2;
        gMappedTexIndices[gTexIndexCount + 4] = base + 3;
        gMappedTexIndices[gTexIndexCount + 5] = base;

        gTexIndexCount += 6;
        gTexVertCount  += 4;

        gTexBatches.back().vertCount = gTexVertCount - gTexBatches.back().vertOffset;
        gTexBatches.back().idxCount  = gTexIndexCount - gTexBatches.back().idxOffset;
    }

    void SDL_GPU_Renderer::UnloadTex(Texture* texture) {
        if (!texture) return;
        if (texture->handle) {
            auto* data = static_cast<SDLGPUTexData*>(texture->handle);
            if (data->sampler) SDL_ReleaseGPUSampler(gDevice, data->sampler);
            if (data->gpuTex)  SDL_ReleaseGPUTexture(gDevice, data->gpuTex);
            delete data;
        }
        delete texture;
    }

    void SDL_GPU_Renderer::ChangeCameraPos(float X, float Y, float zoom) {
        gCamera = { X, Y, zoom };
    }

    void SDL_GPU_Renderer::SetClearColor(float r, float g, float b, float a) {
        const bool looksLikeByteColor =
            (r > 1.0f) || (g > 1.0f) || (b > 1.0f) || (a > 1.0f);

        if (looksLikeByteColor) {
            r /= 255.0f;
            g /= 255.0f;
            b /= 255.0f;
            a /= 255.0f;
        }

        auto clamp01 = [](float v) {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        };

        gClearColor = { clamp01(r), clamp01(g), clamp01(b), clamp01(a) };
    }

    int SDL_GPU_Renderer::Debug_GetVertCount() {
        return static_cast<int>(gVertCount);
    }

    int SDL_GPU_Renderer::Debug_GetIndexCount() {
        return static_cast<int>(gIndexCount);
    }

    int SDL_GPU_Renderer::Debug_GetTexIndexCount() {
        return static_cast<int>(gTexIndexCount);
    }

    int SDL_GPU_Renderer::Debug_GetTexVertCount() {
        return static_cast<int>(gTexIndexCount);
    }

    Camera2D* SDL_GPU_Renderer::GetCamera() {
        return &gCamera;
    }

    Texture* SDL_GPU_Renderer::GetErrorTexture() {
        static Texture errorTexture;

        static SDLGPUTexData data;
        data.gpuTex  = gErrorTex;
        data.sampler = gErrorSampler;

        errorTexture.handle  = &data;
        errorTexture.width   = 8;
        errorTexture.height  = 8;
        errorTexture.format  = TextureFormat::RGBA8;
        errorTexture.backend = RendererBackend::Vulkan;

        return &errorTexture;
    }

    SDL_GPU_Renderer::SDL_GPU_Renderer(RendererBackend backend, CE::VFS::VFS* vfs) {
        gBackend = backend;
        gVFS = vfs;
    }

    void SDL_GPU_Renderer::SetVSync(bool setting) {
        SDL_Window* window = SDL_GetWindowFromID(mWindowID);
        if (setting) {
            SDL_SetGPUSwapchainParameters(
                gDevice,
                window,
                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                SDL_GPU_PRESENTMODE_VSYNC   // VSync on
            );
        } else {
            SDL_SetGPUSwapchainParameters(
                gDevice,
                window,
                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                SDL_GPU_PRESENTMODE_IMMEDIATE   // VSync off
            );
        }
    }
}
