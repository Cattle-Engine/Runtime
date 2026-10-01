#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDLGPURenderer::BeginMode2D() {
        if (mCommandBuffer == nullptr || mSwapchainTexture == nullptr) {
            return;
        }
        if (mMode3DActive) {
            EndMode3D();
        }

        Setup2DCamera();
        mVertexCount = 0;
        mIndexCount = 0;
        mMode2DActive = Map2DBatchBuffers();
    }

    void SDLGPURenderer::EndMode2D() {
        Flush2D();
        mMode2DActive = false;
    }

    void SDLGPURenderer::Flush2D() {
        Unmap2DBatchBuffers();

        if (mVertexCount == 0 || mIndexCount == 0 || mCommandBuffer == nullptr) {
            return;
        }

        SDL_GPUCopyPass* copy_pass =
            SDL_BeginGPUCopyPass(mCommandBuffer);
        if (copy_pass == nullptr) {
            return;
        }

        SDL_GPUTransferBufferLocation vertex_source {};
        vertex_source.transfer_buffer = mVertexUploadBuffer.Get();
        vertex_source.offset = 0;

        SDL_GPUBufferRegion vertex_destination {};
        vertex_destination.buffer = mVertexBuffer.Get();
        vertex_destination.offset = 0;
        vertex_destination.size =
            static_cast<Uint32>(
                mVertexCount * sizeof(detail::Vertex)
            );

        SDL_UploadToGPUBuffer(
            copy_pass,
            &vertex_source,
            &vertex_destination,
            true
        );

        SDL_GPUTransferBufferLocation index_source {};
        index_source.transfer_buffer = mIndexUploadBuffer.Get();
        index_source.offset = 0;

        SDL_GPUBufferRegion index_destination {};
        index_destination.buffer = mIndexBuffer.Get();
        index_destination.offset = 0;
        index_destination.size =
            static_cast<Uint32>(
                mIndexCount * sizeof(uint16_t)
            );

        SDL_UploadToGPUBuffer(
            copy_pass,
            &index_source,
            &index_destination,
            true
        );

        SDL_EndGPUCopyPass(copy_pass);

        // TODO: Begin a 2D render pass, bind mDefault2DPipeline, upload
        // m2DCameraUniform, bind textures, and draw the queued sprite commands.

        mVertexCount = 0;
        mIndexCount = 0;
    }

    void SDLGPURenderer::Draw2DQuad(
        const glm::vec3& position,
        const glm::vec2& size,
        const glm::vec4& colour,
        GPUTexture* texture,
        float rotation) {

        detail::RenderCommand command;
        command.type = detail::RenderCommand::Type::Draw2D;
        command.data = detail::RenderCommand::SpriteDrawCommand {
            .texture = texture,
            .position = position,
            .size = size,
            .colour = colour,
            .rotation = rotation
        };

        mRenderCommands.push_back(command);
    }

    void SDLGPURenderer::DrawRect(
        float x,
        float y,
        float w,
        float h,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a,
        float rotation
    ) {
        if (!mMode2DActive) {
            return;
        }
        Draw2DQuad(
            glm::vec3(x, y, 0.0f),
            glm::vec2(w, h),
            glm::vec4(
                static_cast<float>(r) / 255.0f,
                static_cast<float>(g) / 255.0f,
                static_cast<float>(b) / 255.0f,
                static_cast<float>(a) / 255.0f
            ),
            &mWhiteTexture,
            rotation
        );
    }
}
