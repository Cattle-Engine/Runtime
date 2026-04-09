#include "engine/renderer.hpp"

namespace CE::Renderer::Vulkan {
    class VulkanRenderer : public CE::Renderer::IRenderer {
        public:
            void PreWinInit() override;
        
            void Init(SDL_Window* window) override;
            void Shutdown() override;

            void BeginFrame(SDL_Window* window) override;
            

            void EndFrame(SDL_Window* window) override;

        private:
            SDL_GPUDevice* gDevice = nullptr;
            SDL_GPUCommandBuffer* gCommandBuffer = nullptr;
            SDL_GPURenderPass* gRenderPass = nullptr;
            SDL_GPUBuffer* gVertexBuffer = nullptr;
            SDL_GPUGraphicsPipeline* gPipeline = nullptr;
            
    };
}