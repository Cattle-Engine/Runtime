#include "engine/renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CE::Renderer::Vulkan {
    class VulkanRenderer : public CE::Renderer::IRenderer {
        public:
            void PreWinInit() override;
        
            void Init(SDL_Window* window) override;
            void Shutdown() override;

            void ChangeCameraPos(float X, float Y, float zoom) override;
        
            void DrawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void SetClearColor(float r, float g, float b, float a) override;

            void BeginFrame(SDL_Window* window) override;
            void EndFrame(SDL_Window* window) override;

        private:
            SDL_GPUDevice* gDevice = nullptr;
            SDL_GPUCommandBuffer* gCommandBuffer = nullptr;
            SDL_GPURenderPass* gRenderPass = nullptr;
            SDL_GPUBuffer* gVertexBuffer = nullptr;
            SDL_GPUGraphicsPipeline* gPipeline = nullptr;
            Camera2D gCamera;
            static constexpr size_t MAX_VERTS   = 10000;
            static constexpr size_t MAX_INDICES = 15000;

            SDL_GPUBuffer*         gIndexBuffer   = nullptr;
            SDL_GPUTransferBuffer* gTransferVerts = nullptr;
            SDL_GPUTransferBuffer* gTransferIdx   = nullptr;
            CE::Renderer::Vertex*   gMappedVerts   = nullptr;
            uint16_t* gMappedIndices = nullptr;
            uint32_t  gVertCount     = 0;
            uint32_t  gIndexCount    = 0;
            SDL_FColor gClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            glm::mat4 gMVP{};
    };
}