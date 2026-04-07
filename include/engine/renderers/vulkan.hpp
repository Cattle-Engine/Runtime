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
            SDL_GPUShader* gVertexShader = nullptr;
            SDL_GPUShader* gFragmentShader = nullptr;
            SDL_GPUBuffer* gUniformBuffer = nullptr;
            SDL_GPUGraphicsPipeline* gPipeline = nullptr;
            SDL_GPUColorTargetDescription colorTargetDesc{};
            
            struct Vertex {
                float x, y, z;   
                float r, g, b, a;
            };

            Vertex vertices[3] = {
                {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex (red)
                {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left (yellow)
                {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right (green)
            };
    };
}