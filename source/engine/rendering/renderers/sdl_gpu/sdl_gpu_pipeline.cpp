#include <vector>

#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        struct VertexShaderUserData {
            glm::mat4 model{1.0f};
            glm::mat4 customMat4{1.0f};
            glm::vec4 customVec4[8]{};
            glm::ivec4 customInt4[4]{};
        };

    } // namespace

    SDL_GPUGraphicsPipeline* SDL_GPU_Renderer::CreateGraphicsPipeline(SDL_Window* window, SDL_GPUShader* vertexShader,
                                                                      SDL_GPUShader* fragmentShader) const {
        if (!mDevice || !window || !vertexShader || !fragmentShader) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateGraphicsPipeline received invalid input");
            return nullptr;
        }

        SDL_GPUColorTargetDescription colorDesc{};
        // 2D draw calls target mRenderTexture, not the swapchain. Pipelines must
        // declare the format of their actual render-pass target; using the
        // swapchain format here makes the draws invalid whenever it differs from
        // the fixed offscreen R8G8B8A8 texture (and leaves it black).
        colorDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

        SDL_GPUColorTargetBlendState blend{};
        blend.enable_blend = true;
        blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blend.color_write_mask =
            SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
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
        attrs[2].offset = (sizeof(float) * 3) + (sizeof(uint8_t) * 4);

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

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(mDevice, &pipelineCreateInfo);
        if (!pipeline) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create graphics pipeline: {}", SDL_GetError());
        }

        return pipeline;
    }

    int SDL_GPU_Renderer::CreateDefaultPipeline(SDL_Window* window) {
        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Loading default vertex shader");
        mDefaultVertexShader = Utils::LoadShader(mDevice, "standard_vertex.vert", 0, 1, 0, 0, mVFS);
        if (!mDefaultVertexShader) {
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default vertex shader");
            return 4;
        }

        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Loading default fragment shader");
        mDefaultFragmentShader = Utils::LoadShader(mDevice, "standard_fragment.frag", 1, 1, 0, 0, mVFS);
        if (!mDefaultFragmentShader) {
            SDL_ReleaseGPUShader(mDevice, mDefaultVertexShader);
            mDefaultVertexShader = nullptr;
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default fragment shader");
            return 5;
        }

        CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Creating default graphics pipeline");
        mPipeline = CreateGraphicsPipeline(window, mDefaultVertexShader, mDefaultFragmentShader);

        if (!mPipeline) {
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Failed to create default pipeline");
            return 6;
        }

        return 0;
    }

    void SDL_GPU_Renderer::DestroyDefaultPipeline() {
        if (mPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(mDevice, mPipeline);
            mPipeline = nullptr;
        }
        if (mDefaultVertexShader) {
            SDL_ReleaseGPUShader(mDevice, mDefaultVertexShader);
            mDefaultVertexShader = nullptr;
        }
        if (mDefaultFragmentShader) {
            SDL_ReleaseGPUShader(mDevice, mDefaultFragmentShader);
            mDefaultFragmentShader = nullptr;
        }
    }

    int SDL_GPU_Renderer::CreateUpscalePipeline(SDL_Window* window) {
        mUpscaleVertexShader = Utils::LoadShader(mDevice, "upscale_vertex.vert", 0, 0, 0, 0, mVFS);
        mUpscaleFragmentShader = Utils::LoadShader(mDevice, "upscale_fragment.frag", 1, 0, 0, 0, mVFS);
        if (!mUpscaleVertexShader || !mUpscaleFragmentShader) {
            DestroyUpscalePipeline();
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to load upscale shaders");
            return 1;
        }

        SDL_GPUColorTargetDescription colorDesc{};
        colorDesc.format = SDL_GetGPUSwapchainTextureFormat(mDevice, window);
        colorDesc.blend_state.color_write_mask =
            SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
            SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorDesc;
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineInfo.vertex_shader = mUpscaleVertexShader;
        pipelineInfo.fragment_shader = mUpscaleFragmentShader;
        mUpscalePipeline = SDL_CreateGPUGraphicsPipeline(mDevice, &pipelineInfo);
        if (!mUpscalePipeline) {
            DestroyUpscalePipeline();
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create upscale pipeline: {}", SDL_GetError());
            return 1;
        }
        return 0;
    }

    void SDL_GPU_Renderer::DestroyUpscalePipeline() {
        if (mUpscalePipeline) {
            SDL_ReleaseGPUGraphicsPipeline(mDevice, mUpscalePipeline);
            mUpscalePipeline = nullptr;
        }
        if (mUpscaleVertexShader) {
            SDL_ReleaseGPUShader(mDevice, mUpscaleVertexShader);
            mUpscaleVertexShader = nullptr;
        }
        if (mUpscaleFragmentShader) {
            SDL_ReleaseGPUShader(mDevice, mUpscaleFragmentShader);
            mUpscaleFragmentShader = nullptr;
        }
    }

    void SDL_GPU_Renderer::BindActivePipeline() {
        if (!mRenderPass) {
            return;
        }

        SDL_GPUGraphicsPipeline* pipeline = mPipeline;
        if (mCurrentShader && mCurrentShader->Pipeline) {
            pipeline = mCurrentShader->Pipeline;
        }

        if (pipeline) {
            SDL_BindGPUGraphicsPipeline(mRenderPass, pipeline);
        }
    }

    void SDL_GPU_Renderer::PushActiveShaderUniforms() {
        const SDL_GPU_Renderer_Shader* program = mCurrentShader;
        const glm::mat4& mvp = (program && program->HasOverrideMVP) ? program->OverrideMVP : mMVP;

        SDL_PushGPUVertexUniformData(mCommandBuffer, 0, &mvp, sizeof(mvp));

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

            SDL_PushGPUVertexUniformData(mCommandBuffer, 1, &vertexUserData, sizeof(vertexUserData));
        }

        FragmentShaderUserData fragmentUserData{};
        if (program) {
            fragmentUserData.tint = program->Tint;
            fragmentUserData.resolution = program->Resolution;
            fragmentUserData.misc = program->Misc;
            for (size_t i = 0; i < program->CustomVec4.size(); ++i) {
                fragmentUserData.customVec4[i] = program->CustomVec4[i];
            }
            for (size_t i = 0; i < program->CustomInt4.size(); ++i) {
                fragmentUserData.customInt4[i] = program->CustomInt4[i];
            }

        }
        SDL_PushGPUFragmentUniformData(mCommandBuffer, 0, &fragmentUserData, sizeof(fragmentUserData));
    }

    void SDL_GPU_Renderer::BindShaderSamplers(SDL_GPUTexture* drawTexture, SDL_GPUSampler* drawSampler) {
        const size_t samplerCount = mCurrentShader ? std::max<size_t>(1, mCurrentShader->FragmentSamplerCount) : 1;
        std::vector<SDL_GPUTextureSamplerBinding> bindings(samplerCount);

        for (size_t slot = 0; slot < samplerCount; ++slot) {
            bindings[slot].texture = mWhiteTex;
            bindings[slot].sampler = mWhiteSampler;
        }

        bindings[0].texture = drawTexture ? drawTexture : mWhiteTex;
        bindings[0].sampler = drawSampler ? drawSampler : mWhiteSampler;

        if (mCurrentShader) {
            for (size_t slot = 0; slot < samplerCount; ++slot) {
                if (slot < mCurrentShader->BoundTextures.size()) {
                    Texture* texture = mCurrentShader->BoundTextures[slot];
                    if (texture && texture->handle) {
                        auto* texData = static_cast<SDLGPUTexData*>(texture->handle);
                        if (texData && texData->gpuTex) {
                            bindings[slot].texture = texData->gpuTex;
                            bindings[slot].sampler = texData->sampler ? texData->sampler : mWhiteSampler;
                        }
                    }
                }
            }
        }

        SDL_BindGPUFragmentSamplers(mRenderPass, 0, bindings.data(), static_cast<Uint32>(samplerCount));
    }
} // namespace CE::Renderer::SDL_GPU_Renderer
