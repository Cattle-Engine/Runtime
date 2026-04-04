#include <SDL3/SDL.h>

#include "engine/renderers/vulkan.hpp"
#include "engine/renderer.hpp"
#include "engine/core.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"

constexpr float inv255 = 1.0f / 255.0f;

namespace CE::Renderer::Vulkan {
    class VulkanRenderer : public CE::Renderer::IRenderer {
        public:
            void PreWinInit() override {
                return;
            }

            void Init(SDL_Window* window) override {
                gDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,
                    CE::Core::debugMode, "vulkan");

                if (gDevice == nullptr) {
                    CE::Log(LogLevel::Fatal, "[Vulkan] Unable to find gpu with vulkan support");
                    ShowError("[Vulkan] Unable to find a gpu with vulkan support");
                    std::exit(1);
                }

                if (!SDL_ClaimWindowForGPUDevice(gDevice, window)) {
                    CE::Log(LogLevel::Fatal, "[Vulkan] Unable to bind window to GPU: {}", SDL_GetError());
                    ShowError("[Vulkan] Unable to bind window to gpu device");
                    std::exit(1); 
                }
            }

            void Shutdown() override {
                SDL_DestroyGPUDevice(gDevice);
            }

            void BeginFrame(SDL_Window* window) override {
                gCommandBuffer = SDL_AcquireGPUCommandBuffer(gDevice);
                SDL_GPUTexture* swapchaintexture;
                Uint32 width, height;
                SDL_WaitAndAcquireGPUSwapchainTexture(gCommandBuffer, window, &swapchaintexture, &width, &height);

                if (swapchaintexture == nullptr) {
                    SDL_SubmitGPUCommandBuffer(gCommandBuffer);
                    return;
                }

                SDL_GPUColorTargetInfo colorTargetInfo{};
                colorTargetInfo.clear_color = {240/255.0f, 240/255.0f, 240/255.0f, 255/255.0f};
                colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
                colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
                colorTargetInfo.texture = swapchaintexture;

                gRenderPass = SDL_BeginGPURenderPass(gCommandBuffer, &colorTargetInfo, 1, NULL);
            }

            void EndFrame(SDL_Window* window) override {
                SDL_EndGPURenderPass(gRenderPass);
                SDL_SubmitGPUCommandBuffer(gCommandBuffer);
            }

        private:
            SDL_GPUDevice* gDevice = nullptr;
            SDL_GPUCommandBuffer* gCommandBuffer;
            SDL_GPURenderPass* gRenderPass;
            
            struct Vertex {
                float x, y, z;   
                float r, g, b, a;
            };

            Vertex vertices[3] {
                {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex
                {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left vertex
                {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right vertex
            };
    };

}