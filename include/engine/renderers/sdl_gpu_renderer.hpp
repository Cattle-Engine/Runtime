#include "engine/renderer.hpp"
#include "engine/common/fs/vfs.hpp"

#include "imgui/imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>


namespace CE::Renderer::SDL_GPU_Renderer {
    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo);
    void DestroyGPUDevice(GPUDeviceHandle device);
    // Internal struct 
    // Stores the GPU-side texture + sampler behind Texture::handle.
    struct SDLGPUTexData {
        SDL_GPUTexture* gpuTex  = nullptr;
        SDL_GPUSampler* sampler = nullptr;
    };

    struct TexVertexBatch {
        SDLGPUTexData* texture = nullptr;

        uint32_t vertOffset = 0;
        uint32_t vertCount  = 0;

        uint32_t idxOffset  = 0;
        uint32_t idxCount   = 0;
    };

    class SDL_GPU_Renderer : public CE::Renderer::IRenderer {
        public:
            SDL_GPU_Renderer(RendererBackend backend, CE::VFS::VFS* vfs);

            void PreWinInit() override;
        
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;
            int Shutdown(SDL_Window* window) override;

            void ChangeCameraPos(float X, float Y, float zoom) override;
        
            void DrawRect(float x, float y, float w, float h,
                   uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                       float rotation) override;
            
            void DrawTriangle(
                float x0, float y0,
                float x1, float y1,
                float x2, float y2,
                uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                float rotation) override;

            void DrawCircle(float cx, float cy, float radius,
                                     int segments,
                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void DrawLine(float x1, float y1, float x2, float y2,
                                   float thickness,
                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void SetClearColor(float r, float g, float b, float a) override;

            Texture* LoadTex(const char* path) override;
            Texture* CreateTextureFromData(
                                int width,
                                int height,
                                const void* pixels,
                                TextureFormat format,
                                int pitch = 0,
                                TextureFilter filter = TextureFilter::Linear,
                                TextureWrap wrap = TextureWrap::Clamp
                            ) override;
            void DrawTex(Texture* texture, float x, float y,
                                    float w, float h, Colour colour,
                                    float rotation) override;
            void DrawTexUV(Texture* tex,
                    float x, float y,
                    float w, float h,
                    float u0, float v0,
                    float u1, float v1,
                    Colour colour,
                    float rotation) override;
            void UnloadTex(Texture* texture) override;
            void DrawRectLines(float x, float y, float w, float h,
                                        float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
            void DrawCircleLines(float cx, float cy, float radius,
                                          int segments, float thickness,
                                          uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

            int BeginFrame(SDL_Window* window) override;
            int EndFrame(SDL_Window* window) override;

            Texture* GetErrorTexture() override;
            void* GetNativeTextureHandle(Texture* texture) override;

            int Debug_GetVertCount() override;
            int Debug_GetIndexCount() override;
            int Debug_GetTexIndexCount() override;
            int Debug_GetTexVertCount() override;
            Camera2D* GetCamera() override;

            void SetVSync(bool setting) override;

            void ImGuiStartFrame() override;
            void ImGuiEndFrame(SDL_Window* window) override;

        private:
            ImGuiContext* mImguicontext;
            ImDrawData* mPendingImGuiDrawData = nullptr;
            void ImGuiInit(SDL_Window* window, SDL_GPUDevice* device);
            void ImGuiShutdown();

            SDL_GPUDevice* gDevice = nullptr;
            SDL_WindowID mWindowID;
            SDL_GPUCommandBuffer* gCommandBuffer = nullptr;
            SDL_GPURenderPass* gRenderPass = nullptr;
            SDL_GPUTexture* gSwapchainTexture = nullptr;
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
            CE::Renderer::Vertex* gMappedTexVerts   = nullptr;
            uint16_t*             gMappedTexIndices = nullptr;
            uint32_t              gTexIndexCount    = 0;
            SDL_GPUBuffer*         gTexVertexBuffer = nullptr;
            SDL_GPUBuffer*         gTexIndexBuffer  = nullptr;
            SDL_GPUTransferBuffer* gTransferTexVerts = nullptr;
            SDL_GPUTransferBuffer* gTransferTexIdx   = nullptr;
            std::vector<TexVertexBatch> gTexBatches;
            uint32_t gTexVertCount  = 0;
            SDLGPUTexData* gCurrentTex = nullptr;
            glm::mat4 gMVP{};
            SDL_GPUTexture* gErrorTex = nullptr;
            SDL_GPUSampler* gErrorSampler = nullptr;
            RendererBackend gBackend;
            CE::VFS::VFS* gVFS;
    };
}

namespace CE::Renderer::SDL_GPU_Renderer::Utils {
    SDL_GPUShader* LoadShader( 
        SDL_GPUDevice* device,
        const std::string& shaderfilename,
        Uint32 samplercount,
        Uint32 uniformbuffercount,
        Uint32 storagebuffercount,
        Uint32 storagetexturecount,
        CE::VFS::VFS* vfs,
        const std::string& basePath = "/shaders/"
    );
    glm::mat4 GetView(const Camera2D& cam);
    glm::mat4 GetProjection(float width, float height);
    glm::mat4 GetCameraMatrix(const Camera2D& cam, float w, float h);
}

namespace CE::Renderer::SDL_GPU_Renderer::ImGuiImpl {
    void ImGuiInit(SDL_Window* window, SDL_GPUDevice* device);
    void ImGuiNewFrame();
    void Shutdown();
}
