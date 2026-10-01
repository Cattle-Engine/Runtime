#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDLGPURenderer::Setup2DCamera() {
        const float width = std::max(pRenderSize.x, 1.0f);
        const float height = std::max(pRenderSize.y, 1.0f);

        const glm::mat4 projection = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
        glm::mat4 view{1.0f};
        view = glm::translate(view, glm::vec3(-mCamera2D.x, -mCamera2D.y, 0.0f));
        view = glm::scale(view, glm::vec3(mCamera2D.zoom, mCamera2D.zoom, 1.0f));

        m2DMVP = projection * view;
        m2DCameraUniform.viewProjection = m2DMVP;
        m2DCameraUniform.position = glm::vec4(mCamera2D.x, mCamera2D.y, 0.0f, 1.0f);
    }

    void SDLGPURenderer::Setup3DCamera() {
        const Camera3D& camera = GetCamera3DState();
        const float windowAspect = std::max(pRenderSize.x, 1.0f) / std::max(pRenderSize.y, 1.0f);
        const float aspect = camera.aspectOverride > 0.0001f ? camera.aspectOverride : windowAspect;

        glm::mat4 view{1.0f};
        if (camera.useTarget) {
            view = glm::lookAt(camera.position, camera.target, camera.up);
        } else {
            const glm::mat4 cameraWorld = glm::translate(glm::mat4(1.0f), camera.position)
                * glm::mat4_cast(glm::quat(camera.rotation));
            view = glm::inverse(cameraWorld);
        }

        glm::mat4 projection{1.0f};
        if (camera.projection == Camera3D::ProjectionMode::Orthographic) {
            const float halfWidth = camera.orthoSize * aspect;
            projection = glm::ortho(-halfWidth, halfWidth, -camera.orthoSize, camera.orthoSize,
                                   camera.nearClip, camera.farClip);
        } else {
            projection = glm::perspective(camera.fov, aspect, camera.nearClip, camera.farClip);
        }

        m3DMVP = projection * view;
        m3DCameraUniform.viewProjection = m3DMVP;
        m3DCameraUniform.position = glm::vec4(camera.position, 1.0f);
    }

    bool SDLGPURenderer::Map2DBatchBuffers() {
        if (mMappedVertices != nullptr || mMappedIndices != nullptr) {
            return mMappedVertices != nullptr && mMappedIndices != nullptr;
        }

        mMappedVertices = static_cast<detail::Vertex*>(
            SDL_MapGPUTransferBuffer(mGPUDevice, mVertexUploadBuffer.Get(), true));
        mMappedIndices = static_cast<uint16_t*>(
            SDL_MapGPUTransferBuffer(mGPUDevice, mIndexUploadBuffer.Get(), true));
        if (mMappedVertices != nullptr && mMappedIndices != nullptr) {
            return true;
        }

        CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to map 2D batch buffers: {}", SDL_GetError());
        Unmap2DBatchBuffers();
        return false;
    }

    void SDLGPURenderer::Unmap2DBatchBuffers() {
        if (mMappedVertices != nullptr) {
            SDL_UnmapGPUTransferBuffer(mGPUDevice, mVertexUploadBuffer.Get());
            mMappedVertices = nullptr;
        }
        if (mMappedIndices != nullptr) {
            SDL_UnmapGPUTransferBuffer(mGPUDevice, mIndexUploadBuffer.Get());
            mMappedIndices = nullptr;
        }
    }

    void SDLGPURenderer::ResetFrameState() {
        Unmap2DBatchBuffers();
        mVertexCount = 0;
        mIndexCount = 0;
        mRenderCommands.clear();
        mMode2DActive = false;
        mMode3DActive = false;
    }

    int SDLGPURenderer::BeginFrame(SDL_Window* window) {
        if (mCommandBuffer != nullptr) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] BeginFrame called while a frame is already active");
            return 1;
        }

        mWindow = window != nullptr ? window : mWindow;
        mCommandBuffer = SDL_AcquireGPUCommandBuffer(mGPUDevice);
        if (mCommandBuffer == nullptr) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to acquire GPU command buffer: {}", SDL_GetError());
            return 1;
        }

        int win_w = 0;
        int win_h = 0;
        SDL_GetWindowSizeInPixels(mWindow, &win_w, &win_h);
        pRenderSize = glm::vec2(static_cast<float>(win_w), static_cast<float>(win_h));
        Setup2DCamera();
        Setup3DCamera();
        ResetFrameState();

        mSwapchainTexture = nullptr;
        if(!SDL_WaitAndAcquireGPUSwapchainTexture(mCommandBuffer, mWindow, &mSwapchainTexture, nullptr, nullptr)) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to acquire swapchain texture: {}", SDL_GetError());
            SDL_CancelGPUCommandBuffer(mCommandBuffer);
            mCommandBuffer = nullptr;
            return 2;
        }

        if (mSwapchainTexture == nullptr) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Swapchain texture was nullptr!");
            SDL_CancelGPUCommandBuffer(mCommandBuffer);
            mCommandBuffer = nullptr;
            return 3;
        }

        return 0;
    }

    int SDLGPURenderer::EndFrame([[maybe_unused]] SDL_Window* window) {
        if (mCommandBuffer == nullptr || mSwapchainTexture == nullptr) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] EndFrame called without an active frame");
            return 1;
        }

        // A caller may omit EndMode2D; still finish its upload before presenting.
        if (mMode2DActive || mMappedVertices != nullptr || mMappedIndices != nullptr) {
            Flush2D();
        }
        if (mMode3DActive) {
            EndMode3D();
        }

        SDL_GPUColorTargetInfo color_target{};
        color_target.texture = mSwapchainTexture;
        color_target.clear_color = mClearColour;
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(mCommandBuffer, &color_target, 1, nullptr);
        if (render_pass != nullptr) {
            SDL_EndGPURenderPass(render_pass);
        }

        if (!SDL_SubmitGPUCommandBuffer(mCommandBuffer)) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to submit GPU command buffer: {}", SDL_GetError());
        }

        mCommandBuffer = nullptr;
        mSwapchainTexture = nullptr;
        ResetFrameState();
        return 0;
    }
}
