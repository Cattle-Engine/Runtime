#include <SDL3/SDL.h>
#include <limits>
#include <cassert>

#include "engine/renderers/vulkan.hpp"
#include "engine/renderer.hpp"
#include "engine/core.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/common/bootstrap.hpp"

namespace CE::Renderer::Vulkan {

    void VulkanRenderer::PreWinInit() {
        return;
    }

    void VulkanRenderer::Init(SDL_Window* window) {
        gDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,
            CE::Core::debugMode, "vulkan");

        if (gDevice == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Unable to find gpu with vulkan support");
            ShowError("[Vulkan] Unable to find a gpu with vulkan support");
            CE::Shutdown::DurBootstrapShutdown();
        }

        if (!SDL_ClaimWindowForGPUDevice(gDevice, window)) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Unable to bind window to GPU: {}", SDL_GetError());
            ShowError("[Vulkan] Unable to bind window to gpu device");
            CE::Shutdown::DurBootstrapShutdown();
        }

        CE::Log(LogLevel::Info, "[Vulkan] Loading vertex shader");
        SDL_GPUShader* vertexShader = Utils::LoadShader(gDevice, "standard_vertex.vert", 0, 1, 0, 0);
        if (vertexShader == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[Vulkan] Failed to create vertex shader!");
            CE::Shutdown::DurBootstrapShutdown();
        }

        CE::Log(LogLevel::Info, "[Vulkan] Loading fragment shader");
        SDL_GPUShader* fragmentShader = Utils::LoadShader(gDevice, "standard_fragment.frag", 0, 0, 0, 0);
        if (fragmentShader == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[Vulkan] Failed to create fragment shader!");
            CE::Shutdown::DurBootstrapShutdown();
        }

        // Colour target
        SDL_GPUColorTargetDescription colorDesc{};
        colorDesc.format = SDL_GetGPUSwapchainTextureFormat(gDevice, window);

        // Vertex buffer layout
        SDL_GPUVertexBufferDescription vbDesc{};
        vbDesc.slot = 0;
        vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vbDesc.instance_step_rate = 0;
        vbDesc.pitch = sizeof(Vertex);

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
        pipelineCreateInfo.target_info.num_color_targets        = 1;
        pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;

        pipelineCreateInfo.vertex_input_state.num_vertex_buffers      = 1;
        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes   = 3;
        pipelineCreateInfo.vertex_input_state.vertex_attributes       = attrs;

        pipelineCreateInfo.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.vertex_shader   = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;

        CE::Log(LogLevel::Info, "[Vulkan] Creating graphics pipeline");
        gPipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineCreateInfo);
        if (!gPipeline) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to create pipeline: {}", SDL_GetError());
            CE::Shutdown::DurBootstrapShutdown();
        }

        SDL_ReleaseGPUShader(gDevice, vertexShader);
        SDL_ReleaseGPUShader(gDevice, fragmentShader);
        CE::Log(LogLevel::Info, "[Vulkan] Created graphics pipeline");

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

        CE::Log(LogLevel::Info, "[Vulkan] Batch buffers created");
    }

    void VulkanRenderer::Shutdown() {
        if (gVertexBuffer)  SDL_ReleaseGPUBuffer(gDevice, gVertexBuffer);
        if (gIndexBuffer)   SDL_ReleaseGPUBuffer(gDevice, gIndexBuffer);
        if (gTransferVerts) SDL_ReleaseGPUTransferBuffer(gDevice, gTransferVerts);
        if (gTransferIdx)   SDL_ReleaseGPUTransferBuffer(gDevice, gTransferIdx);
        if (gPipeline)      SDL_ReleaseGPUGraphicsPipeline(gDevice, gPipeline);
        if (gDevice)        SDL_DestroyGPUDevice(gDevice);
    }

    void VulkanRenderer::BeginFrame(SDL_Window* window) {
        gCommandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        if (gCommandBuffer == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to acquire command buffer");
            return;
        }

        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);
        gMVP = CE::Renderer::Utils::GetCameraMatrix(gCamera, (float)winW, (float)winH);

        SDL_GPUTexture* swapchainTexture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(gCommandBuffer, window, &swapchainTexture, NULL, NULL)) {
            CE::Log(LogLevel::Error, "[Vulkan] Failed to acquire swapchain texture: {}", SDL_GetError());
            return;
        }

        if (swapchainTexture == nullptr) return;

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.texture     = swapchainTexture;
        colorTargetInfo.clear_color = gClearColor;
        colorTargetInfo.load_op     = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op    = SDL_GPU_STOREOP_STORE;

        gRenderPass = SDL_BeginGPURenderPass(gCommandBuffer, &colorTargetInfo, 1, NULL);

        SDL_BindGPUGraphicsPipeline(gRenderPass, gPipeline);
        SDL_PushGPUVertexUniformData(gCommandBuffer, 0, &gMVP, sizeof(glm::mat4));

        // Map batch buffers for this frame
        gMappedVerts   = (Vertex*)SDL_MapGPUTransferBuffer(gDevice, gTransferVerts, true);
        gMappedIndices = (uint16_t*)SDL_MapGPUTransferBuffer(gDevice, gTransferIdx, true);
        gVertCount     = 0;
        gIndexCount    = 0;
    }

    void VulkanRenderer::DrawRect(float x, float y, float w, float h,
                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (gVertCount + 4 > MAX_VERTS || gIndexCount + 6 > MAX_INDICES) {
            CE::Log(LogLevel::Warn, "[Vulkan] Batch full, skipping rect");
            return;
        }

        uint16_t base = gVertCount;

        gMappedVerts[base + 0] = { x,     y,     0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 1] = { x + w, y,     0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 2] = { x + w, y + h, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 3] = { x,     y + h, 0, r, g, b, a, 0, 0 };

        gMappedIndices[gIndexCount++] = base;
        gMappedIndices[gIndexCount++] = base + 1;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 2;
        gMappedIndices[gIndexCount++] = base + 3;
        gMappedIndices[gIndexCount++] = base;

        gVertCount += 4;
    }

    void VulkanRenderer::DrawTriangle(
        float x0, float y0,
        float x1, float y1,
        float x2, float y2,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (gVertCount + 3 > MAX_VERTS || gIndexCount + 3 > MAX_INDICES) {
            CE::Log(LogLevel::Warn, "[Vulkan] Batch full, skipping triangle");
            return;
        }

        uint16_t base = gVertCount;

        gMappedVerts[base + 0] = { x0, y0, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 1] = { x1, y1, 0, r, g, b, a, 0, 0 };
        gMappedVerts[base + 2] = { x2, y2, 0, r, g, b, a, 0, 0 };

        gMappedIndices[gIndexCount++] = base;
        gMappedIndices[gIndexCount++] = base + 1;
        gMappedIndices[gIndexCount++] = base + 2;

        gVertCount += 3;
    }

    void VulkanRenderer::EndFrame(SDL_Window* window) {
        SDL_UnmapGPUTransferBuffer(gDevice, gTransferVerts);
        SDL_UnmapGPUTransferBuffer(gDevice, gTransferIdx);

        if (gIndexCount > 0) {
            size_t vSize = sizeof(Vertex) * gVertCount;
            size_t iSize = sizeof(uint16_t) * gIndexCount;

            assert(vSize <= std::numeric_limits<Uint32>::max());
            assert(iSize <= std::numeric_limits<Uint32>::max());

            SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(gDevice);
            SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(uploadCmd);

            SDL_GPUTransferBufferLocation vLoc{ gTransferVerts, 0 };
            SDL_GPUBufferRegion vReg{
                gVertexBuffer,
                0,
                static_cast<Uint32>(vSize)
            };
            SDL_UploadToGPUBuffer(copy, &vLoc, &vReg, true);

            SDL_GPUTransferBufferLocation iLoc{ gTransferIdx, 0 };
            SDL_GPUBufferRegion iReg {
                gIndexBuffer,
                0,
                static_cast<Uint32>(iSize)
            };
            SDL_UploadToGPUBuffer(copy, &iLoc, &iReg, true);

            SDL_EndGPUCopyPass(copy);
            SDL_SubmitGPUCommandBuffer(uploadCmd);

            SDL_GPUBufferBinding vBind{ gVertexBuffer, 0 };
            SDL_GPUBufferBinding iBind{ gIndexBuffer,  0 };
            SDL_BindGPUVertexBuffers(gRenderPass, 0, &vBind, 1);
            SDL_BindGPUIndexBuffer(gRenderPass, &iBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_DrawGPUIndexedPrimitives(gRenderPass, gIndexCount, 1, 0, 0, 0);
        }

        SDL_EndGPURenderPass(gRenderPass);
        SDL_SubmitGPUCommandBuffer(gCommandBuffer);
    }

    void VulkanRenderer::ChangeCameraPos(float X, float Y, float zoom) {
        gCamera = { X, Y, zoom };
    }

    void VulkanRenderer::SetClearColor(float r, float g, float b, float a) {
        gClearColor = { r, g, b, a };
    }
}