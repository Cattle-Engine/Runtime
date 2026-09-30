#include <SDL3/SDL_gpu.h>
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        constexpr SDL_GPUBufferCreateInfo kVertexBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(CE::Renderer::SDL_GPU_Renderer::detail::Vertex) * detail::kMaxVertices,
            .props = 0
        };

        constexpr SDL_GPUBufferCreateInfo kIndexBufferCreateInfo {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(uint16_t) * detail::kMaxIndices,
            .props = 0
        };

        constexpr SDL_GPUTransferBufferCreateInfo kVertexUploadBufferCreateInfo {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(CE::Renderer::SDL_GPU_Renderer::detail::Vertex) * detail::kMaxVertices,
            .props = 0
        };

        constexpr SDL_GPUTransferBufferCreateInfo kIndexUploadBufferCreateInfo {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(uint16_t) * detail::kMaxIndices,
            .props = 0
        };
    }

    int SDLGPURenderer::Init(SDL_Window* window, [[maybe_unused]] bool debug, GPUDeviceHandle gdevice) {
        mWindow = window;
        switch (gdevice->backend) {
        case RendererBackend::DX12:
        case RendererBackend::Metal:
        case RendererBackend::Vulkan:
            break;

        default:
            CE_LOG(LogLevel::Fatal, "[SDL_GPU Renderer] Unsupported GPU device was given!");
            CE_LOG(LogLevel::Info, "[SDL_GPU Renderer] Backend given was: {}", static_cast<int>(gdevice->backend));
            return 1;
        }

        mGPUDevice = static_cast<SDL_GPUDevice*>(gdevice->device);

        if (mGPUDevice == nullptr) {
            CE_LOG(LogLevel::Fatal, "[SDLGPURenderer] mGPUDevice is nullptr!");
            return 2;
        }

        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Claiming window for GPU device");
        if (!SDL_ClaimWindowForGPUDevice(mGPUDevice, window)) {
            CE_LOG(LogLevel::Fatal, "[SDLGPURenderer] Failed to claime window for GPU device");
            return 3;
        }

        const int pipeline_2d_result = Bootstrap_CreateDefault2DPipeline();
        if (pipeline_2d_result != 0) {
            return 4;
        }

        // create the buffers. Create info is stored in a anonymous namespace above
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating batch buffers");

        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Creating vertex and index buffers");
        mVertexBuffer = GPUBuffer(mGPUDevice, SDL_CreateGPUBuffer(mGPUDevice, &kVertexBufferCreateInfo));
        mIndexBuffer = GPUBuffer(mGPUDevice, SDL_CreateGPUBuffer(mGPUDevice, &kIndexBufferCreateInfo));
        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Created vertex and index buffers");

        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Creating vertex and index upload buffers");
        mVertexUploadBuffer = GPUTransferBuffer(mGPUDevice, SDL_CreateGPUTransferBuffer(mGPUDevice, &kVertexUploadBufferCreateInfo));
        mIndexUploadBuffer = GPUTransferBuffer(mGPUDevice, SDL_CreateGPUTransferBuffer(mGPUDevice, &kIndexUploadBufferCreateInfo));
        CE_LOG(LogLevel::Debug, "[SDLGPURenderer] Created vertex and index upload buffers");

        if (
            !mVertexBuffer.IsValid() ||
            !mIndexBuffer.IsValid() ||
            !mVertexUploadBuffer.IsValid() ||
            !mIndexUploadBuffer.IsValid()
        ) {
            CE_LOG(
                LogLevel::Fatal,
                "[SDLGPURenderer] Failed to create batch buffers"
            );

            return 5;
        }

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Created batch buffers");

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating default white texture");
        uint8_t white[4] = {
            255, 255, 255, 255
        };

        mWhiteTexture = BasicCreateTextureFromData(
            1,
            1,
            white
        );

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Created default white texture");
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating error texture");
        uint8_t error_texture[8 * 8 * 4];

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                const bool checker = ((x + y) % 2) == 0;

                uint8_t* pixel = &error_texture[((y * 8) + x) * 4];

                if (checker) {
                    pixel[0] = 255;
                    pixel[1] = 0;
                    pixel[2] = 255;
                    pixel[3] = 255;
                } else {
                    pixel[0] = 0;
                    pixel[1] = 0;
                    pixel[2] = 0;
                    pixel[3] = 255;
                }
            }
        }

        mErrorTexture = BasicCreateTextureFromData(
            8,
            8,
            error_texture
        );

        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Created error texture");
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Creating default normal texture");

        uint8_t default_normal[4] = {
            128, 128, 255, 255
        };

        mDefaultNormalTexture = BasicCreateTextureFromData(
            1,
            1,
            default_normal
        );
        CE_LOG(LogLevel::Info, "[SDLGPURenderer] Created default normal texture");

        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(mWindow, &w, &h);

        pRenderSize.x = static_cast<float>(w);
        pRenderSize.y = static_cast<float>(h); 

        return 0;
    }
}