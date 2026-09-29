#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    SDLGPURenderer::Texture SDLGPURenderer::BasicCreateTextureFromData(
        int width,
        int height,
        const void* pixels
    ) {
        SDL_GPUTextureCreateInfo texture_info{
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .props = 0
        };

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(mGPUDevice, &texture_info);

        if (!texture) {
            return {};
        }

        const size_t data_size =
            static_cast<size_t>(width) *
            static_cast<size_t>(height) *
            4;

        SDL_GPUTransferBufferCreateInfo transfer_info {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = static_cast<uint32_t>(data_size),
            .props = 0
        };

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(mGPUDevice, &transfer_info );

        if (!transfer_buffer) {
            SDL_ReleaseGPUTexture(mGPUDevice, texture);
            return {};
        }

        void* mapped_data = SDL_MapGPUTransferBuffer(
            mGPUDevice,
            transfer_buffer,
            false
        );

        if (!mapped_data) {
            SDL_ReleaseGPUTransferBuffer(mGPUDevice, transfer_buffer);

            SDL_ReleaseGPUTexture(mGPUDevice,  texture);

            return {};
        }

        SDL_memcpy(mapped_data, pixels, data_size);

        SDL_UnmapGPUTransferBuffer(mGPUDevice, transfer_buffer);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(mGPUDevice);

        if (!command_buffer) {
            SDL_ReleaseGPUTransferBuffer(mGPUDevice, transfer_buffer
            );

            SDL_ReleaseGPUTexture(mGPUDevice, texture);

            return {};
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTextureTransferInfo source {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        };

        SDL_GPUTextureRegion destination {
            .texture = texture,
            .mip_level = 0,
            .layer = 0,
            .x = 0,
            .y = 0,
            .z = 0,
            .w = static_cast<uint32_t>(width),
            .h = static_cast<uint32_t>(height),
            .d = 1
        };

        SDL_UploadToGPUTexture(
            copy_pass,
            &source,
            &destination,
            false
        );

        SDL_EndGPUCopyPass(copy_pass);

        if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
            SDL_ReleaseGPUTransferBuffer(mGPUDevice, transfer_buffer);
            SDL_ReleaseGPUTexture(mGPUDevice, texture);
            return {};
        }

        SDL_ReleaseGPUTransferBuffer(mGPUDevice, transfer_buffer);
        return Texture(mGPUDevice, texture);
    }
}