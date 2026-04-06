#include <SDL3/SDL.h>

#include "engine/renderers/vulkan.hpp"
#include "engine/renderer.hpp"
#include "engine/core.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/renderers/shaders/shader_frag_spv.h"
#include "engine/renderers/shaders/shader_vert_spv.h"

constexpr float inv255 = 1.0f / 255.0f;

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
            std::exit(1);
        }

        if (!SDL_ClaimWindowForGPUDevice(gDevice, window)) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Unable to bind window to GPU: {}", SDL_GetError());
            ShowError("[Vulkan] Unable to bind window to gpu device");
            std::exit(1);
        }

        // Setup color target format
        colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(gDevice, window);

        // Create vertex buffer
        SDL_GPUBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexBufferInfo.size = sizeof(vertices);
        vertexBufferInfo.props = 0;

        gVertexBuffer = SDL_CreateGPUBuffer(gDevice, &vertexBufferInfo);
        if (gVertexBuffer == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to create vertex buffer");
            std::exit(1);
        }

        // Upload vertex data
        SDL_GPUTransferBufferCreateInfo transferInfo{};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = sizeof(vertices);
        transferInfo.props = 0;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gDevice, &transferInfo);
        void* transferData = SDL_MapGPUTransferBuffer(gDevice, transferBuffer, false);
        SDL_memcpy(transferData, vertices, sizeof(vertices));
        SDL_UnmapGPUTransferBuffer(gDevice, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);

        SDL_GPUTransferBufferLocation transferLocation{};
        transferLocation.transfer_buffer = transferBuffer;
        transferLocation.offset = 0;

        SDL_GPUBufferRegion bufferRegion{};
        bufferRegion.buffer = gVertexBuffer;
        bufferRegion.offset = 0;
        bufferRegion.size = sizeof(vertices);

        SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);

        SDL_ReleaseGPUTransferBuffer(gDevice, transferBuffer);

        // Create shaders
        SDL_GPUShaderCreateInfo vertexShaderInfo{};
        vertexShaderInfo.code = shader_vert_spv;
        vertexShaderInfo.code_size = shader_vert_spv_len;
        vertexShaderInfo.entrypoint = "main";
        vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertexShaderInfo.num_samplers = 0;
        vertexShaderInfo.num_storage_buffers = 0;
        vertexShaderInfo.num_storage_textures = 0;
        vertexShaderInfo.num_uniform_buffers = 0;

        gVertexShader = SDL_CreateGPUShader(gDevice, &vertexShaderInfo);

        if (gVertexShader == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to create vertex shader");
            std::exit(1);
        }

        SDL_GPUShaderCreateInfo fragmentShaderInfo{};
        fragmentShaderInfo.code = shader_frag_spv;
        fragmentShaderInfo.code_size = shader_frag_spv_len;
        fragmentShaderInfo.entrypoint = "main";
        fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        fragmentShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragmentShaderInfo.num_samplers = 0;
        fragmentShaderInfo.num_storage_buffers = 0;
        fragmentShaderInfo.num_storage_textures = 0;
        fragmentShaderInfo.num_uniform_buffers = 0;

        gFragmentShader = SDL_CreateGPUShader(gDevice, &fragmentShaderInfo);

        if (gFragmentShader == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to create fragment shader");
            std::exit(1);
        }

        // Create graphics pipeline
        SDL_GPUVertexBufferDescription vertexDesc{};
        vertexDesc.slot = 0;
        vertexDesc.pitch = sizeof(Vertex);
        vertexDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexDesc.instance_step_rate = 0;

        SDL_GPUVertexAttribute attributes[2];
        attributes[0].location = 0;
        attributes[0].buffer_slot = 0;
        attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attributes[0].offset = offsetof(Vertex, x);

        attributes[1].location = 1;
        attributes[1].buffer_slot = 0;
        attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        attributes[1].offset = offsetof(Vertex, r);

        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.vertex_shader = gVertexShader;
        pipelineInfo.fragment_shader = gFragmentShader;
        pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexDesc;
        pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineInfo.vertex_input_state.vertex_attributes = attributes;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.num_color_targets = 1;

        gPipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineInfo);

        if (gPipeline == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to create graphics pipeline");
            std::exit(1);
        }
    }

    void VulkanRenderer::Shutdown() {
        if (gPipeline) SDL_ReleaseGPUGraphicsPipeline(gDevice, gPipeline);
        if (gVertexShader) SDL_ReleaseGPUShader(gDevice, gVertexShader);
        if (gFragmentShader) SDL_ReleaseGPUShader(gDevice, gFragmentShader);
        if (gVertexBuffer) SDL_ReleaseGPUBuffer(gDevice, gVertexBuffer);
        SDL_DestroyGPUDevice(gDevice);
    }

    void VulkanRenderer::BeginFrame(SDL_Window* window) {
        gCommandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        if (gCommandBuffer == nullptr) {
            CE::Log(LogLevel::Error, "[Vulkan] Failed to acquire command buffer");
            return;
        }

        SDL_GPUTexture* swapchainTexture;
        Uint32 width, height;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(gCommandBuffer, window, &swapchainTexture, &width, &height)) {
            CE::Log(LogLevel::Error, "[Vulkan] Failed to acquire swapchain texture");
            SDL_SubmitGPUCommandBuffer(gCommandBuffer);
            return;
        }

        if (swapchainTexture == nullptr) {
            SDL_SubmitGPUCommandBuffer(gCommandBuffer);
            return;
        }

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.clear_color = {240.0f * inv255, 240.0f * inv255, 240.0f * inv255, 255.0f * inv255};
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.texture = swapchainTexture;

        gRenderPass = SDL_BeginGPURenderPass(gCommandBuffer, &colorTargetInfo, 1, nullptr);
        if (gRenderPass == nullptr) {
            CE::Log(LogLevel::Error, "[Vulkan] Failed to begin render pass");
            SDL_SubmitGPUCommandBuffer(gCommandBuffer);
            return;
        }

        SDL_BindGPUGraphicsPipeline(gRenderPass, gPipeline);
        SDL_GPUBufferBinding vertexBinding{};
        vertexBinding.buffer = gVertexBuffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(gRenderPass, 0, &vertexBinding, 1);
        SDL_DrawGPUPrimitives(gRenderPass, 3, 1, 0, 0);
    }

    void VulkanRenderer::EndFrame(SDL_Window* /*window*/) {
        if (gRenderPass) {
            SDL_EndGPURenderPass(gRenderPass);
            gRenderPass = nullptr;
        }

        if (gCommandBuffer) {
            SDL_SubmitGPUCommandBuffer(gCommandBuffer);
            gCommandBuffer = nullptr;
        }
    }
}