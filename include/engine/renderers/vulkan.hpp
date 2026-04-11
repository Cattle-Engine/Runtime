#include "engine/renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace CE::Renderer::Vulkan {
    // Internal struct 
    // Stores the GPU-side texture + sampler behind Texture::handle.
    struct VulkanTexData {
        SDL_GPUTexture* gpuTex  = nullptr;
        SDL_GPUSampler* sampler = nullptr;
    };

    struct TexVertexBatch {
        VulkanTexData* texture = nullptr;

        uint32_t vertOffset = 0;
        uint32_t vertCount  = 0;

        uint32_t idxOffset  = 0;
        uint32_t idxCount   = 0;
    };

    class VulkanRenderer : public CE::Renderer::IRenderer {
        public:
            void PreWinInit() override;
        
            void Init(SDL_Window* window) override;
            void Shutdown() override;

            void ChangeCameraPos(float X, float Y, float zoom) override;
        
            void DrawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            
            void DrawTriangle(
                float x0, float y0,
                float x1, float y1,
                float x2, float y2,
                uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

            void DrawCircle(float cx, float cy, float radius,
                                     int segments,
                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void DrawLine(float x1, float y1, float x2, float y2,
                                   float thickness,
                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a);
            void SetClearColor(float r, float g, float b, float a) override;

            Texture* LoadTex(const char* path) override;
            void DrawTex(Texture* texture, float x, float y, float w, float h, Colour colour) override;
            void UnloadTex(Texture* texture) override;
            void DrawRectLines(float x, float y, float w, float h,
                                        float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void DrawCircleLines(float cx, float cy, float radius,
                                          int segments, float thickness,
                                          uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

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
            SDL_GPUTexture*  gWhiteTex     = nullptr;
            SDL_GPUSampler*  gWhiteSampler = nullptr;
            VulkanTexData*   gBoundTex     = nullptr;
            CE::Renderer::Vertex* gMappedTexVerts   = nullptr;
            uint16_t*             gMappedTexIndices = nullptr;
            uint32_t              gTexIndexCount    = 0;
            SDL_GPUBuffer*         gTexVertexBuffer = nullptr;
            SDL_GPUBuffer*         gTexIndexBuffer  = nullptr;
            SDL_GPUTransferBuffer* gTransferTexVerts = nullptr;
            SDL_GPUTransferBuffer* gTransferTexIdx   = nullptr;
            std::vector<TexVertexBatch> gTexBatches;

            Vertex*   gTexVerts  = nullptr;
            uint16_t* gTexIndices = nullptr; 

            uint32_t gTexVertCount  = 0;

            VulkanTexData* gCurrentTex = nullptr;

            glm::mat4 gMVP{};
    };
}