#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        constexpr SDL_GPUTextureUsageFlags kDepthTextureUsage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        constexpr float kSkyboxHalfExtent = 1.001f;
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kHalfPi = kPi * 0.5f;

        struct Camera3DUniformData {
            glm::mat4 viewProjection { 1.0f };
            glm::vec4 cameraPosition { 0.0f, 0.0f, 0.0f, 1.0f };
        };

        struct Model3DUniformData {
            glm::mat4 model { 1.0f };
            glm::mat4 normalMatrix { 1.0f };
            glm::mat4 customMat4 { 1.0f };
            glm::vec4 customVec4[8] {};
            glm::ivec4 customInt4[4] {};
        };

        struct Lighting3DUniformData {
            glm::vec4 sunDirectionEnabled { 0.0f, -1.0f, 0.0f, 1.0f };
            glm::vec4 sunColourIntensity { 1.0f, 1.0f, 1.0f, 1.0f };
            glm::vec4 ambientColourIntensity { 1.0f, 1.0f, 1.0f, 0.2f };
            glm::vec4 materialTint { 1.0f };
            glm::vec4 materialProps { 1.0f, 0.0f, 32.0f, 0.0f };
            glm::vec4 cameraPositionShininess { 0.0f, 0.0f, 0.0f, 32.0f };
            glm::vec4 normalExists { 0.0f }; // .x == 1.0 -> normal map present
            glm::vec4 resolution { 0.0f };
            glm::vec4 misc { 0.0f };
            glm::vec4 customVec4[8] {};
            glm::ivec4 customInt4[4] {};
        };

        struct SkyboxFace {
            const std::shared_ptr<Texture>* texture = nullptr;
            glm::vec3 offset { 0.0f };
            glm::vec3 rotation { 0.0f };
            float textureRotation;
        };

        glm::vec4 ToColourVec4(const Colour& colour) {
            constexpr float kInvByte = 1.0f / 255.0f;
            return glm::vec4(
                static_cast<float>(colour.r) * kInvByte,
                static_cast<float>(colour.g) * kInvByte,
                static_cast<float>(colour.b) * kInvByte,
                static_cast<float>(colour.a) * kInvByte
            );
        }

        glm::quat ToQuaternion(const glm::vec3& eulerRadians) {
            return glm::quat(eulerRadians);
        }

        float RoughnessToShininess(float roughness) {
            const float clamped = std::clamp(roughness, 0.0f, 1.0f);
            return std::max(2.0f, 128.0f - clamped * 120.0f);
        }

        bool HasSkyboxTextures(const CubeMap& skybox) {
            return skybox.front || skybox.back || skybox.left || skybox.right || skybox.top || skybox.bottom;
        }

        std::array<SDL_GPUTexture*, 6> GetSkyboxSourceTextures(const CubeMap& skybox) {
            std::array<SDL_GPUTexture*, 6> sourceTextures {};

            const std::array<const std::shared_ptr<Texture>*, 6> faces = {{
                &skybox.right,
                &skybox.left,
                &skybox.top,
                &skybox.bottom,
                &skybox.front,
                &skybox.back
            }};

            for (size_t i = 0; i < faces.size(); ++i) {
                const std::shared_ptr<Texture>& face = *faces[i];
                if (!face || !face->handle) {
                    continue;
                }

                auto* texData = static_cast<SDLGPUTexData*>(face->handle);
                if (texData && texData->gpuTex) {
                    sourceTextures[i] = texData->gpuTex;
                }
            }

            return sourceTextures;
        }

        SDL_GPUTextureFormat PickDepthFormat(SDL_GPUDevice* device) {
            constexpr std::array<SDL_GPUTextureFormat, 3> kCandidates = {
                SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                SDL_GPU_TEXTUREFORMAT_D24_UNORM,
                SDL_GPU_TEXTUREFORMAT_D16_UNORM
            };

            for (SDL_GPUTextureFormat format : kCandidates) {
                if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, kDepthTextureUsage)) {
                    return format;
                }
            }

            return SDL_GPU_TEXTUREFORMAT_INVALID;
        }
    }

    SDL_GPUGraphicsPipeline* SDL_GPU_Renderer::Create3DGraphicsPipeline(
        SDL_Window* window,
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader,
        bool isSkybox,
        bool isTransparent
    ) const {
        if (!gDevice || !window || !vertexShader || !fragmentShader || gDepthFormat == SDL_GPU_TEXTUREFORMAT_INVALID) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Create3DGraphicsPipeline received invalid input");
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
        vbDesc.pitch = sizeof(GPUVertex3D);

        SDL_GPUVertexAttribute attrs[6]{};

        attrs[0].buffer_slot = 0;
        attrs[0].location = 0;
        attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].offset = static_cast<Uint32>(offsetof(GPUVertex3D, position));

        attrs[1].buffer_slot = 0;
        attrs[1].location = 1;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[1].offset = static_cast<Uint32>(offsetof(GPUVertex3D, normal));

        attrs[2].buffer_slot = 0;
        attrs[2].location = 2;
        attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[2].offset = static_cast<Uint32>(offsetof(GPUVertex3D, color));

        attrs[3].buffer_slot = 0;
        attrs[3].location = 3;
        attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[3].offset = static_cast<Uint32>(offsetof(GPUVertex3D, uv));

        attrs[4].buffer_slot = 0;
        attrs[4].location = 4;
        attrs[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[4].offset = static_cast<Uint32>(offsetof(GPUVertex3D, tangent));

        attrs[5].buffer_slot = 0;
        attrs[5].location = 5;
        attrs[5].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        attrs[5].offset = static_cast<Uint32>(offsetof(GPUVertex3D, tangentSign));

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 6;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = attrs;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineCreateInfo.rasterizer_state.cull_mode = isSkybox ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
        pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        pipelineCreateInfo.rasterizer_state.enable_depth_clip = true;
        pipelineCreateInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write = !(isSkybox || isTransparent);
        pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;
        pipelineCreateInfo.target_info.num_color_targets = 1;
        pipelineCreateInfo.target_info.depth_stencil_format = gDepthFormat;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gDevice, &pipelineCreateInfo);
        if (!pipeline) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create 3D graphics pipeline: {}", SDL_GetError());
        }

        return pipeline;
    }

    bool SDL_GPU_Renderer::EnsureSkyboxMesh() {
        if (gSkyboxMesh) {
            return true;
        }

        if (!gDevice) {
            return false;
        }

        auto* meshData = new SDLGPUMeshData();
        meshData->vertexCount = 4;
        meshData->indexCount = 6;

        const std::array<GPUVertex3D, 4> vertices = {{
            { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0xFFFFFFFF, glm::vec2(1.0f, 1.0f), glm::vec3(1.0f,0.0f,0.0f), 1.0f },
            { glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0xFFFFFFFF, glm::vec2(0.0f, 1.0f), glm::vec3(1.0f,0.0f,0.0f), 1.0f },
            { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0xFFFFFFFF, glm::vec2(0.0f, 0.0f), glm::vec3(1.0f,0.0f,0.0f), 1.0f },
            { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0xFFFFFFFF, glm::vec2(1.0f, 0.0f), glm::vec3(1.0f,0.0f,0.0f), 1.0f },
        }};

        const std::array<uint32_t, 6> indices = { 0, 1, 2, 2, 3, 0 };

        SDL_GPUBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexBufferInfo.size = static_cast<Uint32>(sizeof(GPUVertex3D) * vertices.size());
        meshData->vertexBuffer = SDL_CreateGPUBuffer(gDevice, &vertexBufferInfo);

        SDL_GPUBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexBufferInfo.size = static_cast<Uint32>(sizeof(uint32_t) * indices.size());
        meshData->indexBuffer = SDL_CreateGPUBuffer(gDevice, &indexBufferInfo);

        if (!meshData->vertexBuffer || !meshData->indexBuffer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create skybox mesh buffers");
            if (meshData->vertexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            }
            if (meshData->indexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            }
            delete meshData;
            return false;
        }

        SDL_GPUTransferBufferCreateInfo vertexTransferInfo{};
        vertexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        vertexTransferInfo.size = static_cast<Uint32>(sizeof(GPUVertex3D) * vertices.size());
        SDL_GPUTransferBuffer* vertexTransfer = SDL_CreateGPUTransferBuffer(gDevice, &vertexTransferInfo);

        SDL_GPUTransferBufferCreateInfo indexTransferInfo{};
        indexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        indexTransferInfo.size = static_cast<Uint32>(sizeof(uint32_t) * indices.size());
        SDL_GPUTransferBuffer* indexTransfer = SDL_CreateGPUTransferBuffer(gDevice, &indexTransferInfo);

        if (!vertexTransfer || !indexTransfer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create skybox transfer buffers");
            if (vertexTransfer) {
                SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
            }
            if (indexTransfer) {
                SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);
            }
            SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            delete meshData;
            return false;
        }

        void* mappedVertices = SDL_MapGPUTransferBuffer(gDevice, vertexTransfer, false);
        SDL_memcpy(mappedVertices, vertices.data(), sizeof(GPUVertex3D) * vertices.size());
        SDL_UnmapGPUTransferBuffer(gDevice, vertexTransfer);

        void* mappedIndices = SDL_MapGPUTransferBuffer(gDevice, indexTransfer, false);
        SDL_memcpy(mappedIndices, indices.data(), sizeof(uint32_t) * indices.size());
        SDL_UnmapGPUTransferBuffer(gDevice, indexTransfer);

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        if (!commandBuffer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire command buffer for skybox mesh upload");
            SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
            SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            delete meshData;
            return false;
        }

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to begin copy pass for skybox mesh upload");
            SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
            SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            delete meshData;
            return false;
        }

        SDL_GPUTransferBufferLocation vertexLocation{ vertexTransfer, 0 };
        SDL_GPUBufferRegion vertexRegion{ meshData->vertexBuffer, 0, vertexTransferInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &vertexLocation, &vertexRegion, true);

        SDL_GPUTransferBufferLocation indexLocation{ indexTransfer, 0 };
        SDL_GPUBufferRegion indexRegion{ meshData->indexBuffer, 0, indexTransferInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &indexLocation, &indexRegion, true);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        SDL_WaitForGPUIdle(gDevice);

        SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
        SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);

        gSkyboxMesh = meshData;
        return true;
    }

    bool SDL_GPU_Renderer::EnsureSkyboxCubemap(const CubeMap& skybox) {
        const auto sourceTextures = GetSkyboxSourceTextures(skybox);
        if (std::any_of(sourceTextures.begin(), sourceTextures.end(), [](SDL_GPUTexture* tex) { return tex == nullptr; })) {
            return false;
        }

        const SDL_GPUTexture* referenceTexture = sourceTextures[0];
        if (!referenceTexture) {
            return false;
        }

        Texture* rightFace = skybox.right.get();
        if (!rightFace || rightFace->width <= 0 || rightFace->height <= 0) {
            return false;
        }

        const int faceSize = rightFace->width;
        if (rightFace->height != faceSize) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] Skybox faces are not square, using {}x{} as cubemap size", rightFace->width, rightFace->height);
        }

        const bool cacheValid =
            gSkyboxCubeTexture &&
            gSkyboxCubeSize == faceSize &&
            std::equal(gSkyboxFaceHandles.begin(), gSkyboxFaceHandles.end(), sourceTextures.begin());

        if (cacheValid && gSkyboxCubeSampler) {
            return true;
        }

        DestroySkyboxCubemap();

        SDL_GPUTextureCreateInfo cubeInfo{};
        cubeInfo.type = SDL_GPU_TEXTURETYPE_CUBE;
        cubeInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        cubeInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        cubeInfo.width = static_cast<Uint32>(faceSize);
        cubeInfo.height = static_cast<Uint32>(faceSize);
        cubeInfo.layer_count_or_depth = 6;
        cubeInfo.num_levels = 1;
        cubeInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        gSkyboxCubeTexture = SDL_CreateGPUTexture(gDevice, &cubeInfo);
        if (!gSkyboxCubeTexture) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create skybox cubemap: {}", SDL_GetError());
            return false;
        }

        if (!gSkyboxCubeSampler) {
            SDL_GPUSamplerCreateInfo sampInfo{};
            sampInfo.min_filter = SDL_GPU_FILTER_LINEAR;
            sampInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
            sampInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            sampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            gSkyboxCubeSampler = SDL_CreateGPUSampler(gDevice, &sampInfo);
            if (!gSkyboxCubeSampler) {
                CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create skybox cubemap sampler: {}", SDL_GetError());
                DestroySkyboxCubemap();
                return false;
            }
        }

        SDL_GPUCommandBuffer* commandBuffer = gCommandBuffer ? gCommandBuffer : SDL_AcquireGPUCommandBuffer(gDevice);
        if (!commandBuffer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire command buffer for skybox cubemap upload");
            DestroySkyboxCubemap();
            return false;
        }

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to begin copy pass for skybox cubemap upload");
            if (!gCommandBuffer) {
                SDL_ReleaseGPUTexture(gDevice, gSkyboxCubeTexture);
                gSkyboxCubeTexture = nullptr;
            }
            return false;
        }

        const std::array<SDL_GPUCubeMapFace, 6> faces = {{
            SDL_GPU_CUBEMAPFACE_POSITIVEX,
            SDL_GPU_CUBEMAPFACE_NEGATIVEX,
            SDL_GPU_CUBEMAPFACE_POSITIVEY,
            SDL_GPU_CUBEMAPFACE_NEGATIVEY,
            SDL_GPU_CUBEMAPFACE_POSITIVEZ,
            SDL_GPU_CUBEMAPFACE_NEGATIVEZ
        }};

        for (size_t i = 0; i < faces.size(); ++i) {
            SDL_GPUTextureLocation srcLoc{};
            srcLoc.texture = sourceTextures[i];
            srcLoc.mip_level = 0;
            srcLoc.layer = 0;

            SDL_GPUTextureLocation dstLoc{};
            dstLoc.texture = gSkyboxCubeTexture;
            dstLoc.mip_level = 0;
            dstLoc.layer = static_cast<Uint32>(faces[i]);

            SDL_CopyGPUTextureToTexture(copyPass, &srcLoc, &dstLoc, static_cast<Uint32>(faceSize), static_cast<Uint32>(faceSize), 1, false);
        }

        SDL_EndGPUCopyPass(copyPass);

        if (!gCommandBuffer) {
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            SDL_WaitForGPUIdle(gDevice);
        }

        gSkyboxCubeSize = faceSize;
        gSkyboxFaceHandles = sourceTextures;
        return true;
    }

    void SDL_GPU_Renderer::DestroySkyboxCubemap() {
        if (gSkyboxCubeTexture) {
            SDL_ReleaseGPUTexture(gDevice, gSkyboxCubeTexture);
            gSkyboxCubeTexture = nullptr;
        }

        gSkyboxCubeSize = 0;
        gSkyboxFaceHandles.fill(nullptr);
    }

    void SDL_GPU_Renderer::DestroySkyboxMesh() {
        if (!gSkyboxMesh) {
            return;
        }

        if (gDevice) {
            if (gSkyboxMesh->vertexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, gSkyboxMesh->vertexBuffer);
            }
            if (gSkyboxMesh->indexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, gSkyboxMesh->indexBuffer);
            }
        }

        delete gSkyboxMesh;
        gSkyboxMesh = nullptr;
    }

    int SDL_GPU_Renderer::CreateDefault3DPipeline(SDL_Window* window) {
        gDepthFormat = PickDepthFormat(gDevice);
        if (gDepthFormat == SDL_GPU_TEXTUREFORMAT_INVALID) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] No supported depth format was found");
            return 7;
        }

        gDefault3DVertexShader = Utils::LoadShader(gDevice, "standard_3d.vert", 0, 2, 0, 0, gVFS);
        if (!gDefault3DVertexShader) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to load default 3D vertex shader");
            return 8;
        }

        gDefault3DFragmentShader = Utils::LoadShader(gDevice, "standard_3d.frag", 3, 1, 0, 0, gVFS);
        if (!gDefault3DFragmentShader) {
            SDL_ReleaseGPUShader(gDevice, gDefault3DVertexShader);
            gDefault3DVertexShader = nullptr;
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to load default 3D fragment shader");
            return 9;
        }

        g3DPipeline = Create3DGraphicsPipeline(window, gDefault3DVertexShader, gDefault3DFragmentShader, false, false);
        if (!g3DPipeline) {
            DestroyDefault3DPipeline();
            return 10;
        }

        gTransparent3DPipeline = Create3DGraphicsPipeline(window, gDefault3DVertexShader, gDefault3DFragmentShader, false, true);
        if (!gTransparent3DPipeline) {
            DestroyDefault3DPipeline();
            return 10;
        }

        gSkyboxFragmentShader = Utils::LoadShader(gDevice, "skybox_3d.frag", 1, 1, 0, 0, gVFS);
        if (!gSkyboxFragmentShader) {
            DestroyDefault3DPipeline();
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to load skybox fragment shader");
            return 11;
        }

        gSkyboxPipeline = Create3DGraphicsPipeline(window, gDefault3DVertexShader, gSkyboxFragmentShader, true, false);
        if (!gSkyboxPipeline) {
            DestroyDefault3DPipeline();
            return 12;
        }

        if (!EnsureSkyboxMesh()) {
            DestroyDefault3DPipeline();
            return 13;
        }

        if (!EnsureDepthTexture(window)) {
            DestroyDefault3DPipeline();
            return 14;
        }

        return 0;
    }

    void SDL_GPU_Renderer::DestroyDefault3DPipeline() {
        if (gDepthTexture) {
            SDL_ReleaseGPUTexture(gDevice, gDepthTexture);
            gDepthTexture = nullptr;
        }
        gDepthTextureWidth = 0;
        gDepthTextureHeight = 0;

        if (g3DPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, g3DPipeline);
            g3DPipeline = nullptr;
        }

        if (gTransparent3DPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, gTransparent3DPipeline);
            gTransparent3DPipeline = nullptr;
        }

        if (gSkyboxPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, gSkyboxPipeline);
            gSkyboxPipeline = nullptr;
        }

        if (gSkyboxFragmentShader) {
            SDL_ReleaseGPUShader(gDevice, gSkyboxFragmentShader);
            gSkyboxFragmentShader = nullptr;
        }

        if (gDefault3DVertexShader) {
            SDL_ReleaseGPUShader(gDevice, gDefault3DVertexShader);
            gDefault3DVertexShader = nullptr;
        }

        if (gDefault3DFragmentShader) {
            SDL_ReleaseGPUShader(gDevice, gDefault3DFragmentShader);
            gDefault3DFragmentShader = nullptr;
        }

        DestroySkyboxCubemap();
        DestroySkyboxMesh();
    }

    bool SDL_GPU_Renderer::EnsureDepthTexture(SDL_Window* window) {
        if (!gDevice || !window || gDepthFormat == SDL_GPU_TEXTUREFORMAT_INVALID) {
            return false;
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSize(window, &width, &height);

        width = std::max(width, 1);
        height = std::max(height, 1);

        if (gDepthTexture && gDepthTextureWidth == width && gDepthTextureHeight == height) {
            return true;
        }

        if (gDepthTexture) {
            SDL_ReleaseGPUTexture(gDevice, gDepthTexture);
            gDepthTexture = nullptr;
        }

        SDL_GPUTextureCreateInfo depthInfo{};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = gDepthFormat;
        depthInfo.usage = kDepthTextureUsage;
        depthInfo.width = static_cast<Uint32>(width);
        depthInfo.height = static_cast<Uint32>(height);
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;

        gDepthTexture = SDL_CreateGPUTexture(gDevice, &depthInfo);
        if (!gDepthTexture) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create depth texture: {}", SDL_GetError());
            gDepthTextureWidth = 0;
            gDepthTextureHeight = 0;
            return false;
        }

        gDepthTextureWidth = width;
        gDepthTextureHeight = height;
        return true;
    }

    glm::mat4 SDL_GPU_Renderer::BuildTransformMatrix(const Transform3D& transform) {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
        const glm::mat4 rotation = glm::mat4_cast(ToQuaternion(transform.rotation));
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);
        return translation * rotation * scale;
    }

    glm::mat4 SDL_GPU_Renderer::BuildViewProjectionMatrix(const Camera3D& camera, float aspectRatio) const {
        const float resolvedAspect = camera.aspectOverride > 0.0001f ? camera.aspectOverride : aspectRatio;

        glm::mat4 view { 1.0f };
        if (camera.useTarget) {
            view = glm::lookAt(camera.position, camera.target, camera.up);
        } else {
            const glm::mat4 cameraWorld =
                glm::translate(glm::mat4(1.0f), camera.position) *
                glm::mat4_cast(ToQuaternion(camera.rotation));
            view = glm::inverse(cameraWorld);
        }

        glm::mat4 projection { 1.0f };
        if (camera.projection == Camera3D::ProjectionMode::Orthographic) {
            const float halfWidth = camera.orthoSize * resolvedAspect;
            projection = glm::ortho(
                -halfWidth,
                halfWidth,
                -camera.orthoSize,
                camera.orthoSize,
                camera.nearClip,
                camera.farClip
            );
        } else {
            projection = glm::perspective(camera.fov, resolvedAspect, camera.nearClip, camera.farClip);
        }

        return projection * view;
    }

    GPUMesh* SDL_GPU_Renderer::CreateGPUMesh(MeshData& mesh) {
        if (!gDevice) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateGPUMesh called before Init");
            return nullptr;
        }

        if (mesh.vertices.empty() || mesh.indices.empty()) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] CreateGPUMesh called with an empty mesh");
            return nullptr;
        }

        auto* meshData = new SDLGPUMeshData();
        meshData->vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        meshData->indexCount = static_cast<uint32_t>(mesh.indices.size());

        std::vector<GPUVertex3D> gpuVertices(mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const Vertex3D& src = mesh.vertices[i];
            gpuVertices[i] = GPUVertex3D {
                src.position,
                glm::length(src.normal) > 0.0001f ? glm::normalize(src.normal) : glm::vec3(0.0f, 1.0f, 0.0f),
                // Pack RGBA into a single uint32_t
                (static_cast<uint32_t>(src.color.r)) |
                (static_cast<uint32_t>(src.color.g) << 8) |
                (static_cast<uint32_t>(src.color.b) << 16) |
                (static_cast<uint32_t>(src.color.a) << 24),
                src.uv,
                src.tangent,
                src.tangentSign
            };
        }

        SDL_GPUBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexBufferInfo.size = static_cast<Uint32>(sizeof(GPUVertex3D) * gpuVertices.size());
        meshData->vertexBuffer = SDL_CreateGPUBuffer(gDevice, &vertexBufferInfo);

        SDL_GPUBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexBufferInfo.size = static_cast<Uint32>(sizeof(uint32_t) * mesh.indices.size());
        meshData->indexBuffer = SDL_CreateGPUBuffer(gDevice, &indexBufferInfo);

        if (!meshData->vertexBuffer || !meshData->indexBuffer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create mesh buffers");
            if (meshData->vertexBuffer) SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            if (meshData->indexBuffer) SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            delete meshData;
            return nullptr;
        }

        SDL_GPUTransferBufferCreateInfo vertexTransferInfo{};
        vertexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        vertexTransferInfo.size = static_cast<Uint32>(sizeof(GPUVertex3D) * gpuVertices.size());
        SDL_GPUTransferBuffer* vertexTransfer = SDL_CreateGPUTransferBuffer(gDevice, &vertexTransferInfo);

        SDL_GPUTransferBufferCreateInfo indexTransferInfo{};
        indexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        indexTransferInfo.size = static_cast<Uint32>(sizeof(uint32_t) * mesh.indices.size());
        SDL_GPUTransferBuffer* indexTransfer = SDL_CreateGPUTransferBuffer(gDevice, &indexTransferInfo);

        if (!vertexTransfer || !indexTransfer) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create mesh transfer buffers");
            if (vertexTransfer) SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
            if (indexTransfer) SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            delete meshData;
            return nullptr;
        }

        void* mappedVertices = SDL_MapGPUTransferBuffer(gDevice, vertexTransfer, false);
        SDL_memcpy(mappedVertices, gpuVertices.data(), sizeof(GPUVertex3D) * gpuVertices.size());
        SDL_UnmapGPUTransferBuffer(gDevice, vertexTransfer);

        void* mappedIndices = SDL_MapGPUTransferBuffer(gDevice, indexTransfer, false);
        SDL_memcpy(mappedIndices, mesh.indices.data(), sizeof(uint32_t) * mesh.indices.size());
        SDL_UnmapGPUTransferBuffer(gDevice, indexTransfer);

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

        SDL_GPUTransferBufferLocation vertexLocation{ vertexTransfer, 0 };
        SDL_GPUBufferRegion vertexRegion{ meshData->vertexBuffer, 0, vertexTransferInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &vertexLocation, &vertexRegion, true);

        SDL_GPUTransferBufferLocation indexLocation{ indexTransfer, 0 };
        SDL_GPUBufferRegion indexRegion{ meshData->indexBuffer, 0, indexTransferInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &indexLocation, &indexRegion, true);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        SDL_WaitForGPUIdle(gDevice);

        SDL_ReleaseGPUTransferBuffer(gDevice, vertexTransfer);
        SDL_ReleaseGPUTransferBuffer(gDevice, indexTransfer);

        auto* gpuMesh = new GPUMesh();
        gpuMesh->handle = meshData;
        gpuMesh->vertex_buffer = meshData->vertexBuffer;
        gpuMesh->index_buffer = meshData->indexBuffer;
        gpuMesh->vertex_count = meshData->vertexCount;
        gpuMesh->indice_count = meshData->indexCount;
        return gpuMesh;
    }

    void SDL_GPU_Renderer::DestroyGPUMesh(GPUMesh* mesh) {
        if (!mesh) {
            return;
        }

        auto* meshData = static_cast<SDLGPUMeshData*>(mesh->handle);
        if (meshData && gDevice) {
            if (meshData->vertexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, meshData->vertexBuffer);
            }
            if (meshData->indexBuffer) {
                SDL_ReleaseGPUBuffer(gDevice, meshData->indexBuffer);
            }
            delete meshData;
        }

        delete mesh;
    }

    void SDL_GPU_Renderer::DrawMesh(GPUMesh* mesh, Material& material, const Transform3D& transform, bool error_tex) {
        if (!gFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Can't draw mesh outside of BeginFrame/EndFrame");
                mWarnedOutsideFrame = true;
            }
            return;
        }

        auto* meshData = mesh ? static_cast<SDLGPUMeshData*>(mesh->handle) : nullptr;
        if (!meshData || !meshData->vertexBuffer || !meshData->indexBuffer || meshData->indexCount == 0) {
            return;
        }

        auto* albedoData = material.albedo && material.albedo->handle
            ? static_cast<SDLGPUTexData*>(material.albedo->handle) : nullptr;
        
        auto* normalData = material.normal && material.normal->handle
            ? static_cast<SDLGPUTexData*>(material.normal->handle) : nullptr;
        
        auto* mrData = material.metallicRoughnessTex && material.metallicRoughnessTex->handle
            ? static_cast<SDLGPUTexData*>(material.metallicRoughnessTex->handle) : nullptr;

        const glm::mat4 modelMatrix = BuildTransformMatrix(transform);
        const glm::mat4 normalMatrix = glm::inverseTranspose(modelMatrix);

        MeshDrawCommand command{};
        command.mesh = meshData;
        command.texture = albedoData;  // Slot 0: Albedo
        command.normaltex= normalData; // Slot 1: Normal
        command.mrtex = mrData;        // Slot 2: Metallic-Roughness
        command.sampler = albedoData ? albedoData->sampler : gWhiteSampler;
        command.shader = gCurrentShader;
        command.model = modelMatrix;
        command.normalMatrix = normalMatrix;
        command.tint = ToColourVec4(material.tint);
        command.materialProps = glm::vec4(material.roughness, material.metallic, 0.0f, 0.0f);
        command.isTransparent = material.isTransparent;
        command.distanceToCamera = glm::distance(mCamera3DState.position, transform.position);

        if (command.shader && command.shader->Mode != SDL_GPU_Renderer_Shader::PipelineMode::Mode3D) {
            command.shader->Mode = SDL_GPU_Renderer_Shader::PipelineMode::Mode3D;
            command.shader->Dirty = true;
        }

        gMeshCommands.push_back(command);
    }

    void SDL_GPU_Renderer::DrawMeshMat4(
        GPUMesh* mesh,
        Material& material,
        const glm::mat4& transform,
        bool error_tex
    ) {
        if (!gFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Can't draw mesh outside of BeginFrame/EndFrame");
                mWarnedOutsideFrame = true;
            }
            return;
        }

        auto* meshData = mesh ? static_cast<SDLGPUMeshData*>(mesh->handle) : nullptr;
        if (!meshData || !meshData->vertexBuffer || !meshData->indexBuffer || meshData->indexCount == 0) {
            return;
        }

        auto* albedoData = material.albedo && material.albedo->handle
            ? static_cast<SDLGPUTexData*>(material.albedo->handle) : nullptr;
        
        auto* normalData = material.normal && material.normal->handle
            ? static_cast<SDLGPUTexData*>(material.normal->handle) : nullptr;
        
        auto* mrData = material.metallicRoughnessTex && material.metallicRoughnessTex->handle
            ? static_cast<SDLGPUTexData*>(material.metallicRoughnessTex->handle) : nullptr;

        const glm::mat4 modelMatrix = transform;
        const glm::mat4 normalMatrix = glm::inverseTranspose(modelMatrix);

        MeshDrawCommand command{};
        command.mesh = meshData;
        command.texture = albedoData;  // Slot 0: Albedo
        command.normaltex= normalData; // Slot 1: Normal
        command.mrtex = mrData;        // Slot 2: Metallic-Roughness
        command.sampler = albedoData ? albedoData->sampler : gWhiteSampler;
        command.shader = gCurrentShader;
        command.model = modelMatrix;
        command.normalMatrix = normalMatrix;
        command.tint = ToColourVec4(material.tint);
        command.materialProps = glm::vec4(material.roughness, material.metallic, 0.0f, 0.0f);
        command.isTransparent = material.isTransparent;
        command.distanceToCamera = glm::distance(mCamera3DState.position, glm::vec3(transform[3]));

        if (command.shader && command.shader->Mode != SDL_GPU_Renderer_Shader::PipelineMode::Mode3D) {
            command.shader->Mode = SDL_GPU_Renderer_Shader::PipelineMode::Mode3D;
            command.shader->Dirty = true;
        }

        gMeshCommands.push_back(command);
    }

    void SDL_GPU_Renderer::DrawSkybox(SDL_GPURenderPass* renderPass, const Camera3D& camera, float aspectRatio, int width, int height) {
        if (!renderPass || !gSkyboxPipeline || !gSkyboxMesh || !HasSkyboxTextures(GetSkyBoxState())) {
            return;
        }

        const CubeMap& skybox = GetSkyBoxState();

        const glm::mat4 viewProjection = BuildViewProjectionMatrix(camera, aspectRatio);
        Camera3DUniformData cameraUniform{};
        cameraUniform.viewProjection = viewProjection;
        cameraUniform.cameraPosition = glm::vec4(camera.position, 1.0f);
        SDL_PushGPUVertexUniformData(gCommandBuffer, 0, &cameraUniform, sizeof(cameraUniform));

        SDL_BindGPUGraphicsPipeline(renderPass, gSkyboxPipeline);

        const std::array<SkyboxFace, 6> faces = {{
            { &skybox.front,  { 0.0f, 0.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f },
            { &skybox.back,   { 0.0f, 0.0f, -1.0f }, { 0.0f, glm::radians(180.0f), 0.0f }, 0.0f },
            { &skybox.left,   { -1.0f, 0.0f, 0.0f }, { 0.0f, glm::radians(-90.0f), 0.0f }, 0.0f },
            { &skybox.right,  {  1.0f, 0.0f, 0.0f }, { 0.0f, glm::radians(90.0f), 0.0f }, 0.0f },
            { &skybox.top,    { 0.0f,  1.0f, 0.0f }, { glm::radians(-90.0f), glm::radians(90.0f), 0.0f }, 0.0f },  // Pitch -90°, Yaw +90°
            { &skybox.bottom, { 0.0f, -1.0f, 0.0f }, { glm::radians(90.0f), glm::radians(90.0f), 0.0f }, 0.0f },   // Pitch +90°, Yaw +90°
        }};

        SDL_GPUBufferBinding vertexBinding{ gSkyboxMesh->vertexBuffer, 0 };
        SDL_GPUBufferBinding indexBinding{ gSkyboxMesh->indexBuffer, 0 };
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        for (const auto& face : faces) {
            if (!face.texture) {
                continue;
            }

            const std::shared_ptr<Texture>& faceTexture = *face.texture;
            if (!faceTexture || !faceTexture->handle) {
                continue;
            }

            auto* faceData = static_cast<SDLGPUTexData*>(faceTexture->handle);
            if (!faceData || !faceData->gpuTex) {
                continue;
            }

            Transform3D faceTransform{};
            faceTransform.position = camera.position + face.offset;
            faceTransform.rotation = face.rotation;
            faceTransform.scale = glm::vec3(kSkyboxHalfExtent);

            Model3DUniformData modelUniform{};
            modelUniform.model = BuildTransformMatrix(faceTransform);
            modelUniform.normalMatrix = glm::inverseTranspose(modelUniform.model);
            SDL_PushGPUVertexUniformData(gCommandBuffer, 1, &modelUniform, sizeof(modelUniform));

            Lighting3DUniformData fragmentUniform{};
            fragmentUniform.sunDirectionEnabled = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            fragmentUniform.sunColourIntensity = glm::vec4(0.0f);
            fragmentUniform.ambientColourIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            fragmentUniform.materialTint = glm::vec4(1.0f);
            fragmentUniform.materialProps = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            fragmentUniform.cameraPositionShininess = glm::vec4(camera.position, 0.0f);
            fragmentUniform.normalExists = glm::vec4(0.0f);
            fragmentUniform.resolution = glm::vec4(static_cast<float>(width), static_cast<float>(height), aspectRatio, 0.0f);
            SDL_PushGPUFragmentUniformData(gCommandBuffer, 0, &fragmentUniform, sizeof(fragmentUniform));

            SDL_GPUTextureSamplerBinding binding{};
            binding.texture = faceData->gpuTex;
            binding.sampler = faceData->sampler ? faceData->sampler : gWhiteSampler;
            SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);
            SDL_DrawGPUIndexedPrimitives(renderPass, gSkyboxMesh->indexCount, 1, 0, 0, 0);
        }
    }

    void SDL_GPU_Renderer::ChangeCameraPos3D(const Transform3D& transform) {
        gCamera3DTransform = transform;
        mCamera3DState.position = transform.position;
        mCamera3DState.rotation = transform.rotation;
        mCamera3DState.useTarget = false;
    }

    void SDL_GPU_Renderer::SetCamera3D(const Camera3D& camera) {
        mCamera3DState = camera;
        gCamera3DTransform.position = camera.position;
        gCamera3DTransform.rotation = camera.rotation;
        gCamera3DTransform.scale = glm::vec3(1.0f);
    }

    void SDL_GPU_Renderer::BeginMode3D() {
        m3DModeActive = true;
        m2DModeActive = false;
    }

    void SDL_GPU_Renderer::EndMode3D() {
        m3DModeActive = false;
    }

    void SDL_GPU_Renderer::BeginMode2D() {
        m2DModeActive = true;
        m3DModeActive = false;
    }

    void SDL_GPU_Renderer::EndMode2D() {
        m2DModeActive = false;
    }

    void SDL_GPU_Renderer::DrawQueuedMeshes() {
        if (!gCommandBuffer || !gSwapchainTexture) {
            return;
        }

        SDL_Window* window = SDL_GetWindowFromID(mWindowID);
        if (!window || !EnsureDepthTexture(window) || !g3DPipeline || !gDepthTexture) {
            return;
        }

        int width = 1;
        int height = 1;
        SDL_GetWindowSize(window, &width, &height);
        const float aspectRatio = static_cast<float>(std::max(width, 1)) / static_cast<float>(std::max(height, 1));

        Camera3DUniformData cameraUniform{};
        cameraUniform.viewProjection = BuildViewProjectionMatrix(mCamera3DState, aspectRatio);
        cameraUniform.cameraPosition = glm::vec4(mCamera3DState.position, 1.0f);

        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.texture = gSwapchainTexture;
        colorTargetInfo.clear_color = gClearColor;
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthTargetInfo{};
        depthTargetInfo.texture = gDepthTexture;
        depthTargetInfo.clear_depth = 1.0f;
        depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(gCommandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
        if (!renderPass) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to begin 3D render pass: {}", SDL_GetError());
            return;
        }

        if (HasSkyboxTextures(GetSkyBoxState())) {
            DrawSkybox(renderPass, mCamera3DState, aspectRatio, width, height);
        }

        const LightingState& lighting = GetLightingState();
        
        // Split opaque and transparent
        std::vector<const MeshDrawCommand*> opaqueCommands;
        std::vector<const MeshDrawCommand*> transparentCommands;
        opaqueCommands.reserve(gMeshCommands.size());
        transparentCommands.reserve(gMeshCommands.size());

        for (const auto& cmd : gMeshCommands) {
            if (cmd.isTransparent) {
                transparentCommands.push_back(&cmd);
            } else {
                opaqueCommands.push_back(&cmd);
            }
        }

        // Sort transparent: Back-to-Front
        std::sort(transparentCommands.begin(), transparentCommands.end(), [](const MeshDrawCommand* a, const MeshDrawCommand* b) {
            return a->distanceToCamera > b->distanceToCamera;
        });

        auto DrawCommand = [&](const MeshDrawCommand& command, bool isTransparentMode) {
            if (!command.mesh) return;

            SDL_GPUGraphicsPipeline* activePipeline = isTransparentMode ? gTransparent3DPipeline : g3DPipeline;
            SDL_GPU_Renderer_Shader* activeShader = command.shader;
            
            if (activeShader) {
                if (activeShader->Mode != SDL_GPU_Renderer_Shader::PipelineMode::Mode3D) {
                    activeShader->Mode = SDL_GPU_Renderer_Shader::PipelineMode::Mode3D;
                    activeShader->Dirty = true;
                }

                if (activeShader->Dirty || !activeShader->Pipeline) {
                    auto shaderWrapper = Shader { activeShader, gBackend };
                    if (!CompileShaderProgram(&shaderWrapper)) {
                        CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] Falling back to default 3D shader");
                        activeShader = nullptr;
                    }
                }

                if (activeShader && activeShader->Pipeline) {
                    activePipeline = activeShader->Pipeline;
                }
            }

            SDL_BindGPUGraphicsPipeline(renderPass, activePipeline);

            Model3DUniformData modelUniform{};
            modelUniform.model = command.model;
            modelUniform.normalMatrix = command.normalMatrix;
            if (activeShader) {
                modelUniform.customMat4 = activeShader->CustomMat4;
                for (size_t i = 0; i < activeShader->CustomVec4.size(); ++i) {
                    modelUniform.customVec4[i] = activeShader->CustomVec4[i];
                }
                for (size_t i = 0; i < activeShader->CustomInt4.size(); ++i) {
                    modelUniform.customInt4[i] = activeShader->CustomInt4[i];
                }
            }
            SDL_PushGPUVertexUniformData(gCommandBuffer, 1, &modelUniform, sizeof(modelUniform));

            Lighting3DUniformData fragmentUniform{};
            fragmentUniform.sunDirectionEnabled = glm::vec4(lighting.sun.direction, lighting.sun.enabled ? 1.0f : 0.0f);
            fragmentUniform.sunColourIntensity = glm::vec4(lighting.sun.colour, lighting.sun.intensity);
            fragmentUniform.ambientColourIntensity = glm::vec4(lighting.ambient.colour, lighting.ambient.intensity);
            fragmentUniform.materialTint = activeShader ? activeShader->Tint * command.tint : command.tint;
            fragmentUniform.materialProps = glm::vec4(
                std::clamp(command.materialProps.x, 0.0f, 1.0f),
                std::clamp(command.materialProps.y, 0.0f, 1.0f),
                0.0f, 0.0f
            );
            fragmentUniform.cameraPositionShininess = glm::vec4(mCamera3DState.position, 0.0f);
            fragmentUniform.normalExists = glm::vec4((command.normaltex && command.normaltex->gpuTex) ? 1.0f : 0.0f);
            fragmentUniform.resolution = glm::vec4(static_cast<float>(width), static_cast<float>(height), aspectRatio, 0.0f);
            if (activeShader) {
                fragmentUniform.misc = activeShader->Misc;
                for (size_t i = 0; i < activeShader->CustomVec4.size(); ++i) {
                    fragmentUniform.customVec4[i] = activeShader->CustomVec4[i];
                }
                for (size_t i = 0; i < activeShader->CustomInt4.size(); ++i) {
                    fragmentUniform.customInt4[i] = activeShader->CustomInt4[i];
                }
            }
            SDL_PushGPUFragmentUniformData(gCommandBuffer, 0, &fragmentUniform, sizeof(fragmentUniform));

            const size_t baseSamplerCount = 3;
            const size_t samplerCount = activeShader ? std::max<size_t>(baseSamplerCount, activeShader->FragmentSamplerCount) : baseSamplerCount;
            std::vector<SDL_GPUTextureSamplerBinding> bindings(samplerCount);

            for (size_t slot = 0; slot < samplerCount; ++slot) {
                bindings[slot].texture = gWhiteTex;
                bindings[slot].sampler = gWhiteSampler;
            }

            bindings[0].texture = (command.texture && command.texture->gpuTex) ? command.texture->gpuTex : gWhiteTex;
            bindings[0].sampler = command.sampler ? command.sampler : gWhiteSampler;
            bindings[1].texture = (command.normaltex && command.normaltex->gpuTex) ? command.normaltex->gpuTex : gDefaultNormalTex;
            bindings[1].sampler = command.sampler ? command.sampler : gWhiteSampler;
            bindings[2].texture = (command.mrtex && command.mrtex->gpuTex) ? command.mrtex->gpuTex : gWhiteTex;
            bindings[2].sampler = command.sampler ? command.sampler : gWhiteSampler;

            if (activeShader) {
                for (size_t slot = 0; slot < samplerCount && slot < activeShader->BoundTextures.size(); ++slot) {
                    Texture* texture = activeShader->BoundTextures[slot];
                    if (texture && texture->handle) {
                        auto* texData = static_cast<SDLGPUTexData*>(texture->handle);
                        if (texData && texData->gpuTex) {
                            bindings[slot].texture = texData->gpuTex;
                            bindings[slot].sampler = texData->sampler ? texData->sampler : gWhiteSampler;
                        }
                    }
                }
            }

            SDL_BindGPUFragmentSamplers(renderPass, 0, bindings.data(), static_cast<Uint32>(samplerCount));
            SDL_GPUBufferBinding vertexBinding{ command.mesh->vertexBuffer, 0 };
            SDL_GPUBufferBinding indexBinding{ command.mesh->indexBuffer, 0 };
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_DrawGPUIndexedPrimitives(renderPass, command.mesh->indexCount, 1, 0, 0, 0);
        };

        SDL_PushGPUVertexUniformData(gCommandBuffer, 0, &cameraUniform, sizeof(cameraUniform));

        // 1. Draw Opaque
        for (const auto* cmd : opaqueCommands) {
            DrawCommand(*cmd, false);
        }

        // 2. Draw Transparent
        for (const auto* cmd : transparentCommands) {
            DrawCommand(*cmd, true);
        }

        SDL_EndGPURenderPass(renderPass);
    }
}
