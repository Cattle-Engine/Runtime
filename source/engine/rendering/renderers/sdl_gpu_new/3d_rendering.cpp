#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        constexpr SDL_GPUBufferCreateInfo k3DMaterialBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(glm::vec4),
            .props = 0
        };

        constexpr SDL_GPUBufferCreateInfo k3DCameraBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(detail::CameraUniformData),
            .props = 0
        };
    }

    void SDLGPURenderer::BeginMode3D() {
        if (mCommandBuffer == nullptr || mSwapchainTexture == nullptr) {
            return;
        }
        if (mMode2DActive) {
            EndMode2D();
        }

        Setup3DCamera();
        if (!m3DMaterialBuffer.IsValid()) {
            m3DMaterialBuffer = GPUBuffer(
                mGPUDevice, SDL_CreateGPUBuffer(mGPUDevice, &k3DMaterialBufferCreateInfo));
        }
        if (!m3DCameraBuffer.IsValid()) {
            m3DCameraBuffer = GPUBuffer(
                mGPUDevice, SDL_CreateGPUBuffer(mGPUDevice, &k3DCameraBufferCreateInfo));
        }
        if (!mVertexBuffer.IsValid() || !mIndexBuffer.IsValid() ||
            !m3DMaterialBuffer.IsValid() || !m3DCameraBuffer.IsValid()) {
            return;
        }

        SDL_GPUColorTargetInfo colour_target{};
        colour_target.texture = mSwapchainTexture;
        colour_target.load_op = SDL_GPU_LOADOP_LOAD;
        colour_target.store_op = SDL_GPU_STOREOP_STORE;
        m3DRenderPass = SDL_BeginGPURenderPass(mCommandBuffer, &colour_target, 1, nullptr);
        if (m3DRenderPass == nullptr) {
            return;
        }

        const SDL_GPUBufferBinding vertex_binding{mVertexBuffer.Get(), 0};
        const SDL_GPUBufferBinding index_binding{mIndexBuffer.Get(), 0};
        SDL_BindGPUVertexBuffers(m3DRenderPass, 0, &vertex_binding, 1);
        SDL_BindGPUIndexBuffer(m3DRenderPass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_GPUBuffer* const storage_buffers[] = {
            m3DMaterialBuffer.Get(),
            m3DCameraBuffer.Get()
        };
        SDL_BindGPUVertexStorageBuffers(m3DRenderPass, 0, storage_buffers, 2);
        mMode3DActive = true;

        // TODO: Create a depth target and bind the default 3D pipeline/shaders.
    }

    void SDLGPURenderer::EndMode3D() {
        if (!mMode3DActive) {
            return;
        }

        // TODO: Upload m3DCameraUniform and submit queued mesh draw commands.
        SDL_EndGPURenderPass(m3DRenderPass);
        m3DRenderPass = nullptr;
        mMode3DActive = false;
    }
}
