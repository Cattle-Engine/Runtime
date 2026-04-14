#include <SDL3/SDL.h>

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
        SDL_GPUShader* vertexShader = Utils::LoadShader(gDevice, "positioncolor.vert", 0, 0, 0, 0);
        
        if (vertexShader == nullptr) {
            CE::Log(CE::LogLevel::Fatal, "[Vulkan] Failed to create vertex shader!");
            CE::Shutdown::DurBootstrapShutdown();
        }

        CE::Log(LogLevel::Info, "[Vulkan] Loading fragment shader");
        SDL_GPUShader* fragmentShader = Utils::LoadShader(gDevice, "solidcolour.frag", 0, 0, 0, 0);

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
        SDL_GPUVertexAttribute attrs[2]{};

        // Position, location: 0
        attrs[0].buffer_slot = 0;
        attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].location = 0;
        attrs[0].offset = 0;

        // Colour, location: 1
        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].location = 1;
        attrs[1].offset = sizeof(float) * 3;

        // Pipeline creation info
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.target_info.num_color_targets = 1;
        pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;

        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;

        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 2;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = attrs;

        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;

        // Create pipeline
        CE::Log(LogLevel::Info, "[Vulkan] Creating graphics pipeline");
        gPipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineCreateInfo);

        if (!gPipeline) {
            SDL_Log("Failed to create pipeline: %s", SDL_GetError());
            CE::Shutdown::DurBootstrapShutdown();
        }
        CE::Log(LogLevel::Info, "[Vulkan] Created graphics pipeline");

        // Cleanup the old shaders
         CE::Log(LogLevel::Info, "[Vulkan] Cleaning up shaders");
        SDL_ReleaseGPUShader(gDevice, vertexShader);
        SDL_ReleaseGPUShader(gDevice, fragmentShader);

        SDL_GPUBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexBufferInfo.size = sizeof(Vertex) * 3;
        gVertexBuffer = SDL_CreateGPUBuffer(gDevice, &vertexBufferInfo);

        SDL_GPUTransferBufferCreateInfo transferBufferInfo{};
        transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferBufferInfo.size = sizeof(Vertex) * 3;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gDevice, &transferBufferInfo);

        Vertex* transferData = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(
            gDevice,
            transferBuffer,
            false
        ));
        transferData[0] = (Vertex) {    -1,    -1, 0, 255,   0,   0, 255 };
        transferData[1] = (Vertex) {     1,    -1, 0,   0, 255,   0, 255 };
        transferData[2] = (Vertex) {     0,     1, 0,   0,   0, 255, 255 };

        SDL_UnmapGPUTransferBuffer(gDevice, transferBuffer);

    	// Upload the transfer data to the vertex buffer
        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTransferBufferLocation transferLocation{};
        transferLocation.transfer_buffer = transferBuffer;
        transferLocation.offset = 0;

        SDL_GPUBufferRegion bufferRegion{};
        bufferRegion.buffer = gVertexBuffer;
        bufferRegion.offset = 0;
        bufferRegion.size = sizeof(Vertex) * 3;

        SDL_UploadToGPUBuffer(
            copyPass,
            &transferLocation,
            &bufferRegion,
            false
        );

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(gDevice, transferBuffer);
    }

    void VulkanRenderer::Shutdown() {
        if (gPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, gPipeline);
        }

        if (gVertexBuffer != nullptr) {
            SDL_ReleaseGPUBuffer(gDevice, gVertexBuffer);
        }
        
        if (gDevice != nullptr) {
            SDL_DestroyGPUDevice(gDevice);
        }
    }

    void VulkanRenderer::BeginFrame(SDL_Window* window) {
        gCommandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);

        if (gCommandBuffer == nullptr) {
            CE::Log(LogLevel::Fatal, "[Vulkan] Failed to acquire command buffer");
            return;
        }

        SDL_GPUTexture* swapchaintexture;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(gCommandBuffer, window, &swapchaintexture, NULL, NULL)) {
            CE::Log(LogLevel::Error ,"Failed to acquire swap chain texture failed: {}", SDL_GetError());
        }

        if (swapchaintexture != nullptr) {
            SDL_GPUColorTargetInfo colorTargetInfo{};
            colorTargetInfo.texture = swapchaintexture;
            colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            gRenderPass = SDL_BeginGPURenderPass(
                gCommandBuffer,
                &colorTargetInfo,
                1,
                NULL
            );

            SDL_GPUBufferBinding vertexBinding{};
            vertexBinding.buffer = gVertexBuffer;
            vertexBinding.offset = 0;

            SDL_FColor colors[3] = {
                {1, 0, 0, 1},
                {0, 1, 0, 1},
                {0, 0, 1, 1}
            };



            SDL_BindGPUGraphicsPipeline(gRenderPass, gPipeline);

            SDL_Push

            // SDL_BindGPUVertexBuffers(gRenderPass, 0, &vertexBinding, 1);
            // SDL_DrawGPUPrimitives(gRenderPass, 3, 1, 0, 0);
        }
    }

    void VulkanRenderer::EndFrame(SDL_Window* window) {
        SDL_EndGPURenderPass(gRenderPass);

        if (gCommandBuffer != nullptr) {
            SDL_SubmitGPUCommandBuffer(gCommandBuffer);
        }
    }
}