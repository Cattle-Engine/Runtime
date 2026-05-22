#include <vector>

#include "engine/renderers/sdl_gpu_renderer.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        struct VertexShaderUserData {
            glm::mat4 model { 1.0f };
            glm::mat4 customMat4 { 1.0f };
            glm::vec4 customVec4[8] {};
            glm::ivec4 customInt4[4] {};
        };

        struct FragmentShaderUserData {
            glm::vec4 tint { 1.0f, 1.0f, 1.0f, 1.0f };
            glm::vec4 resolution { 0.0f, 0.0f, 0.0f, 0.0f };
            glm::vec4 misc { 0.0f, 0.0f, 0.0f, 0.0f };
            glm::vec4 customVec4[8] {};
            glm::ivec4 customInt4[4] {};
        };
    }

    SDL_GPUGraphicsPipeline* SDL_GPU_Renderer::CreateGraphicsPipeline(
        SDL_Window* window,
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader
    ) const {
        if (!gDevice || !window || !vertexShader || !fragmentShader) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] CreateGraphicsPipeline received invalid input");
            return nullptr;
        }

        SDL_GPUColorTargetDescription colorDesc{};
        colorDesc.format = SDL_GetGPUSwapchainTextureFormat(gDevice, window);

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

        SDL_GPUVertexBufferDescription vbDesc{};
        vbDesc.slot = 0;
        vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vbDesc.instance_step_rate = 0;
        vbDesc.pitch = sizeof(Vertex);

        SDL_GPUVertexAttribute attrs[3]{};

        attrs[0].buffer_slot = 0;
        attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].location = 0;
        attrs[0].offset = 0;

        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].location = 1;
        attrs[1].offset = sizeof(float) * 3;

        attrs[2].buffer_slot = 0;
        attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].location = 2;
        attrs[2].offset = sizeof(float) * 3 + sizeof(uint8_t) * 4;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.target_info.num_color_targets = 1;
        pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 3;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = attrs;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineCreateInfo);
        if (!pipeline) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] Failed to create graphics pipeline: {}", SDL_GetError());
        }

        return pipeline;
    }

    int SDL_GPU_Renderer::CreateDefaultPipeline(SDL_Window* window) {
        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Loading default vertex shader");
        gDefaultVertexShader = Utils::LoadShader(gDevice, "standard_vertex.vert", 0, 1, 0, 0, gVFS);
        if (!gDefaultVertexShader) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default vertex shader");
            return 4;
        }

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Loading default fragment shader");
        gDefaultFragmentShader = Utils::LoadShader(gDevice, "standard_fragment.frag", 1, 0, 0, 0, gVFS);
        if (!gDefaultFragmentShader) {
            SDL_ReleaseGPUShader(gDevice, gDefaultVertexShader);
            gDefaultVertexShader = nullptr;
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default fragment shader");
            return 5;
        }

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Creating default graphics pipeline");
        gPipeline = CreateGraphicsPipeline(window, gDefaultVertexShader, gDefaultFragmentShader);

        if (!gPipeline) {
            CE::Log(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default pipeline");
            return 6;
        }

        return 0;
    }

    void SDL_GPU_Renderer::DestroyDefaultPipeline() {
        if (gPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, gPipeline);
            gPipeline = nullptr;
        }
        if (gDefaultVertexShader) {
            SDL_ReleaseGPUShader(gDevice, gDefaultVertexShader);
            gDefaultVertexShader = nullptr;
        }
        if (gDefaultFragmentShader) {
            SDL_ReleaseGPUShader(gDevice, gDefaultFragmentShader);
            gDefaultFragmentShader = nullptr;
        }
    }

    void SDL_GPU_Renderer::BindActivePipeline() {
        if (!gRenderPass) {
            return;
        }

        SDL_GPUGraphicsPipeline* pipeline = gPipeline;
        if (gCurrentShader && gCurrentShader->Pipeline) {
            pipeline = gCurrentShader->Pipeline;
        }

        if (pipeline) {
            SDL_BindGPUGraphicsPipeline(gRenderPass, pipeline);
        }
    }

    void SDL_GPU_Renderer::PushActiveShaderUniforms() {
        const SDL_GPU_Renderer_Shader* program = gCurrentShader;
        const glm::mat4& mvp = (program && program->HasOverrideMVP) ? program->OverrideMVP : gMVP;

        SDL_PushGPUVertexUniformData(gCommandBuffer, 0, &mvp, sizeof(mvp));

        if (program && !program->UsesDefaultVertex) {
            VertexShaderUserData vertexUserData{};
            vertexUserData.model = program->ModelMatrix;
            vertexUserData.customMat4 = program->CustomMat4;
            for (size_t i = 0; i < program->CustomVec4.size(); ++i) {
                vertexUserData.customVec4[i] = program->CustomVec4[i];
            }
            for (size_t i = 0; i < program->CustomInt4.size(); ++i) {
                vertexUserData.customInt4[i] = program->CustomInt4[i];
            }

            SDL_PushGPUVertexUniformData(gCommandBuffer, 1, &vertexUserData, sizeof(vertexUserData));
        }

        if (program && !program->UsesDefaultFragment) {
            FragmentShaderUserData fragmentUserData{};
            fragmentUserData.tint = program->Tint;
            fragmentUserData.resolution = program->Resolution;
            fragmentUserData.misc = program->Misc;
            for (size_t i = 0; i < program->CustomVec4.size(); ++i) {
                fragmentUserData.customVec4[i] = program->CustomVec4[i];
            }
            for (size_t i = 0; i < program->CustomInt4.size(); ++i) {
                fragmentUserData.customInt4[i] = program->CustomInt4[i];
            }

            SDL_PushGPUFragmentUniformData(gCommandBuffer, 0, &fragmentUserData, sizeof(fragmentUserData));
        }
    }

    void SDL_GPU_Renderer::BindShaderSamplers(SDL_GPUTexture* drawTexture, SDL_GPUSampler* drawSampler) {
        const size_t samplerCount = gCurrentShader ? std::max<size_t>(1, gCurrentShader->FragmentSamplerCount) : 1;
        std::vector<SDL_GPUTextureSamplerBinding> bindings(samplerCount);

        for (size_t slot = 0; slot < samplerCount; ++slot) {
            bindings[slot].texture = gWhiteTex;
            bindings[slot].sampler = gWhiteSampler;
        }

        bindings[0].texture = drawTexture ? drawTexture : gWhiteTex;
        bindings[0].sampler = drawSampler ? drawSampler : gWhiteSampler;

        if (gCurrentShader) {
            for (size_t slot = 0; slot < samplerCount; ++slot) {
                if (slot < gCurrentShader->BoundTextures.size()) {
                    Texture* texture = gCurrentShader->BoundTextures[slot];
                    if (texture && texture->handle) {
                        auto* texData = static_cast<SDLGPUTexData*>(texture->handle);
                        if (texData && texData->gpuTex) {
                            bindings[slot].texture = texData->gpuTex;
                            bindings[slot].sampler = texData->sampler ? texData->sampler : gWhiteSampler;
                        }
                    }
                }
            }
        }

        SDL_BindGPUFragmentSamplers(gRenderPass, 0, bindings.data(), static_cast<Uint32>(samplerCount));
    }
}
