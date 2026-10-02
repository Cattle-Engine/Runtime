#include <SDL3/SDL_gpu.h>
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    SDLGPURenderer::GraphicsPipeline SDLGPURenderer::CreateGraphicsPipeline(Shader& vertex, Shader& fragment) {
        SDL_GPUColorTargetDescription colour_desc;

        colour_desc.format = SDL_GetGPUSwapchainTextureFormat(mGPUDevice, mWindow);
        SDL_GPUColorTargetBlendState blend{};
        blend.enable_blend = true;
        blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
        colour_desc.blend_state = blend;

        SDL_GPUVertexBufferDescription vb_desc{};
        vb_desc.slot = 0;
        vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vb_desc.instance_step_rate = 0;
        vb_desc.pitch = sizeof(detail::Vertex);

        SDL_GPUVertexAttribute attrs[3]{};
        attrs[0].buffer_slot = 0;
        attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].location = 0;
        attrs[0].offset = 0;

        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        attrs[1].location = 1;
        attrs[1].offset = sizeof(float) * 3;

        attrs[2].buffer_slot = 0;
        attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].location = 2;
        attrs[2].offset = sizeof(float) * 7;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.target_info.num_color_targets = 1;
        pipeline_create_info.target_info.color_target_descriptions = &colour_desc;
        pipeline_create_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_create_info.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
        pipeline_create_info.vertex_input_state.num_vertex_attributes = 3;
        pipeline_create_info.vertex_input_state.vertex_attributes = attrs;
        pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipeline_create_info.vertex_shader = vertex.Get();
        pipeline_create_info.fragment_shader = fragment.Get();

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(mGPUDevice, &pipeline_create_info);
        if (!pipeline) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to create graphics pipeline: {}", SDL_GetError());
        }

        return GraphicsPipeline(mGPUDevice, pipeline);
    }

    int SDLGPURenderer::Bootstrap_CreateDefault2DPipeline() {
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Loading default 2D vertex shader");
        mDefault2DVertexShader = LoadShader("standard_2d.vert", ShaderStage::Vertex, 0, 1, 0, 0);
        if (!mDefault2DVertexShader.IsValid()) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to load default 2D vertex shader!");
            return 1;
        }

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Load default 2D fragment shader");
        mDefault2DFragmentShader = LoadShader("standard_2d.frag", ShaderStage::Fragment, 1, 0, 0 ,0);
        if (!mDefault2DFragmentShader.IsValid()) {
            mDefault2DVertexShader.Reset();
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to load default 2D fragment shader");
            return 2;
        }

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating default 2D pipeline");
        mDefault2DPipeline = CreateGraphicsPipeline(mDefault2DVertexShader, mDefault2DFragmentShader);
        if (!mDefault2DPipeline.IsValid()) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to create default 2D pipeline");
            return 3;
        }

        return 0;
    }

    int SDLGPURenderer::Bootstrap_CreateDefault3DPipeline() {
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Loading default 3D vertex shader");
        mDefault3DVertexShader = LoadShader("standard_3d.vert", ShaderStage::Vertex, 0, 1, 0, 0);
        if (!mDefault3DVertexShader.IsValid()) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to load default 3D vertex shader!");
            return 1;
        }

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Load default 3D fragment shader");
        mDefault3DFragmentShader = LoadShader("standard_3d.frag", ShaderStage::Fragment, 1, 0, 0 ,0);
        if (!mDefault3DFragmentShader.IsValid()) {
            mDefault3DVertexShader.Reset();
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to load default 3D fragment shader");
            return 2;
        }

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating default 3D pipeline");
        mDefault3DPipeline = CreateGraphicsPipeline(mDefault3DVertexShader, mDefault3DFragmentShader);
        if (!mDefault3DPipeline.IsValid()) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to create default 3D pipeline");
            return 3;
        }

        return 0;
    }
}