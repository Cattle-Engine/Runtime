#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "engine/rendering/renderer.hpp"
#include "engine/common/fs/vfs.hpp"

struct ImGuiContext;
struct ImDrawData;

namespace CE::Renderer::SDL_GPU_Renderer {
    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo);
    void DestroyGPUDevice(GPUDeviceHandle device);

    struct SDLGPUTexData {
        SDL_GPUTexture* gpuTex        = nullptr;
        SDL_GPUSampler* sampler       = nullptr;
        SDL_GPUSampler* repeatSampler = nullptr;
    };

    struct SDL_GPU_Renderer_Shader {
        enum class PipelineMode {
            Mode2D,
            Mode3D
        };

        SDL_GPUShader* VertexShader = nullptr;
        SDL_GPUShader* FragmentShader = nullptr;
        SDL_GPUGraphicsPipeline* Pipeline = nullptr;
        bool UsesDefaultVertex = true;
        bool UsesDefaultFragment = true;
        bool Dirty = true;
        std::string VertexPath;
        std::string FragmentPath;
        glm::mat4 OverrideMVP { 1.0f };
        bool HasOverrideMVP = false;
        glm::mat4 ModelMatrix { 1.0f };
        glm::mat4 CustomMat4 { 1.0f };
        glm::vec4 Tint { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 Resolution { 0.0f, 0.0f, 0.0f, 0.0f };
        glm::vec4 Misc { 0.0f, 0.0f, 0.0f, 0.0f };
        std::array<glm::vec4, 8> CustomVec4 {};
        std::array<glm::ivec4, 4> CustomInt4 {};
        Uint32 FragmentSamplerCount = 1;
        std::vector<Texture*> BoundTextures;
        PipelineMode Mode = PipelineMode::Mode2D;
    };

    struct DeferredDeleteEntry {
        SDLGPUTexData* data;
        int framesUntilDelete;

        bool operator==(const DeferredDeleteEntry& other) const {
            return data == other.data;
        }
    };

    struct TexVertexBatch {
        SDLGPUTexData* texture = nullptr;
        SDL_GPUSampler* sampler = nullptr;
        SDL_GPU_Renderer_Shader* shader = nullptr;

        uint32_t vertOffset = 0;
        uint32_t vertCount  = 0;

        uint32_t idxOffset  = 0;
        uint32_t idxCount   = 0;
    };

    struct PrimitiveBatch {
        SDL_GPU_Renderer_Shader* shader = nullptr;
        uint32_t idxOffset = 0;
        uint32_t idxCount = 0;
    };

    #pragma pack(push, 1) 
    struct GPUVertex3D {
        glm::vec3 position { 0.0f };
        glm::vec3 normal { 0.0f, 1.0f, 0.0f };
        uint32_t color = 0xFFFFFFFF; 
        glm::vec2 uv { 0.0f };
        glm::vec3 tangent { 0.0f };
        float tangentSign = 1.0f;
    };
    #pragma pack(pop)

    struct SDLGPUMeshData {
        SDL_GPUBuffer* vertexBuffer = nullptr;
        SDL_GPUBuffer* indexBuffer = nullptr;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    struct MeshDrawCommand {
        SDLGPUMeshData* mesh = nullptr;
        SDLGPUTexData* texture = nullptr;
        SDLGPUTexData* normaltex = nullptr;
        SDLGPUTexData* mrtex = nullptr; // Metallic roughness tex
        SDL_GPUSampler* sampler = nullptr;
        SDL_GPU_Renderer_Shader* shader = nullptr;
        glm::mat4 model { 1.0f };
        glm::mat4 normalMatrix { 1.0f };
        glm::vec4 tint { 1.0f };
        glm::vec4 materialProps { 1.0f, 0.0f, 0.0f, 0.0f };
        bool isTransparent = false;
        float distanceToCamera = 0.0f;
    };

    class SDL_GPU_Renderer : public Renderer::IRenderer {
        public:
            SDL_GPU_Renderer(RendererBackend backend, CE::VFS::VFS* vfs);

            void PreWinInit() override;

            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;
            int Shutdown(SDL_Window* window) override;

            void ChangeCameraPos2D(float X, float Y, float zoom) override;

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
                                    float rotation,
                                    TextureFlip flip = TextureFlip::None) override;
            void DrawTexUV(Texture* tex,
                    float x, float y,
                    float w, float h,
                    float u0, float v0,
                    float u1, float v1,
                    Colour colour,
                    float rotation,
                    TextureFlip flip = TextureFlip::None) override;
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

            Shader* CreateShaderProgram() override;
            Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override;
            bool LoadShaderStage(Shader* shaderProgram, const char* path, ShaderStage stage, int samplerCount = 1) override;
            bool UseDefaultShaderStage(Shader* shaderProgram, ShaderStage stage) override;
            bool CompileShaderProgram(Shader* shaderProgram) override;
            void UnloadShader(Shader* shader) override;

            void BindShader(Shader* shader) override;
            void UnbindShader() override;

            void SetShaderFloat(const char* name, float value) override;
            void SetShaderVec2(const char* name, float x, float y) override;
            void SetShaderVec3(const char* name, float x, float y, float z) override;
            void SetShaderVec4(const char* name, float x, float y, float z, float w) override;

            void SetShaderMat4(const char* name, const float* mat4) override;
            void SetShaderInt(const char* name, int value) override;
            void SetShaderTexture(const char* name, Texture* texture, int slot) override;

            GPUMesh* CreateGPUMesh(MeshData& mesh) override;
            void DestroyGPUMesh(GPUMesh* mesh) override;
            void DrawMesh(GPUMesh* mesh, Material& material, const Transform3D& transform, bool error_tex) override;
            void DrawMeshMat4(
                GPUMesh* mesh,
                Material& material,
                const glm::mat4& transform,
                bool error_tex
            ) override;
            void ChangeCameraPos3D(const Transform3D& transform) override;
            void SetCamera3D(const Camera3D& camera) override;
            void BeginMode3D() override;
            void EndMode3D() override;
            void BeginMode2D() override;
            void EndMode2D() override;

            void ProcessDeferredDeletions();

        private:
            SDL_GPUGraphicsPipeline* CreateGraphicsPipeline(
                SDL_Window* window,
                SDL_GPUShader* vertexShader,
                SDL_GPUShader* fragmentShader
            ) const;
            int CreateDefaultPipeline(SDL_Window* window);
            void DestroyDefaultPipeline();
            void BindActivePipeline();
            void PushActiveShaderUniforms();
            void BindShaderSamplers(SDL_GPUTexture* drawTexture, SDL_GPUSampler* drawSampler);
            int CreateDefault3DPipeline(SDL_Window* window);
            void DestroyDefault3DPipeline();
            SDL_GPUGraphicsPipeline* Create3DGraphicsPipeline(
                SDL_Window* window,
                SDL_GPUShader* vertexShader,
                SDL_GPUShader* fragmentShader,
                bool isSkybox = false,
                bool isTransparent = false
            ) const;
            bool EnsureDepthTexture(SDL_Window* window);
            bool EnsureSkyboxMesh();
            bool EnsureSkyboxCubemap(const CubeMap& skybox);
            void DestroySkyboxCubemap();
            void DestroySkyboxMesh();
            static glm::mat4 BuildTransformMatrix(const Transform3D& transform);
            glm::mat4 BuildViewProjectionMatrix(const Camera3D& camera, float aspectRatio) const;
            void DrawSkybox(SDL_GPURenderPass* renderPass, const Camera3D& camera, float aspectRatio, int width, int height);
            void DrawQueuedMeshes();
            static SDL_GPU_Renderer_Shader* GetShaderProgram(Shader* shaderProgram);
            SDL_GPUShader* GetStageShader(const SDL_GPU_Renderer_Shader* shaderProgram, ShaderStage stage) const;
            bool LoadShaderStageIntoProgram(SDL_GPU_Renderer_Shader* shaderProgram, const char* path, ShaderStage stage, Uint32 samplerCount);
            void ReleaseProgramStage(SDL_GPU_Renderer_Shader* shaderProgram, ShaderStage stage);
            static bool ShaderPathSuggests3D(const std::string& shaderPath);
            static std::string GetShaderBaseName(const char* path);
            static bool ParseIndexedUniformName(const char* name, const char* prefix, size_t maxCount, size_t& indexOut);

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
            SDL_GPUShader* gDefaultVertexShader = nullptr;
            SDL_GPUShader* gDefaultFragmentShader = nullptr;
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
            SDL_GPUTexture* gDefaultNormalTex = nullptr;
            SDL_GPUSampler* gNormalSampler = nullptr;
            CE::Renderer::Vertex* gMappedTexVerts   = nullptr;
            uint16_t* gMappedTexIndices = nullptr;
            uint32_t              gTexIndexCount    = 0;
            SDL_GPUBuffer*         gTexVertexBuffer = nullptr;
            SDL_GPUBuffer*         gTexIndexBuffer  = nullptr;
            SDL_GPUTransferBuffer* gTransferTexVerts = nullptr;
            SDL_GPUTransferBuffer* gTransferTexIdx   = nullptr;
            std::vector<PrimitiveBatch> gPrimitiveBatches;
            std::vector<TexVertexBatch> gTexBatches;
            uint32_t gTexVertCount  = 0;
            SDL_GPU_Renderer_Shader* gCurrentPrimitiveShader = nullptr;
            SDLGPUTexData* gCurrentTex = nullptr;
            SDL_GPUSampler* gCurrentTexSampler = nullptr;
            glm::mat4 gMVP{};
            SDL_GPUTexture* gErrorTex = nullptr;
            SDL_GPUSampler* gErrorSampler = nullptr;
            RendererBackend gBackend;
            bool gFrameActive = false;
            bool mWarnedOutsideFrame = false;
            bool m3DModeActive = false;
            bool m2DModeActive = false;
            CE::VFS::VFS* gVFS;

            std::vector<DeferredDeleteEntry> gDeferredDeletes;
            std::vector<MeshDrawCommand> gMeshCommands;

            SDL_GPU_Renderer_Shader* gCurrentShader = nullptr;
            SDL_GPUGraphicsPipeline* g3DPipeline = nullptr;
            SDL_GPUGraphicsPipeline* gTransparent3DPipeline = nullptr;
            SDL_GPUGraphicsPipeline* gSkyboxPipeline = nullptr;
            SDL_GPUShader* gDefault3DVertexShader = nullptr;
            SDL_GPUShader* gDefault3DFragmentShader = nullptr;
            SDL_GPUShader* gSkyboxFragmentShader = nullptr;
            SDLGPUMeshData* gSkyboxMesh = nullptr;
            SDL_GPUTexture* gSkyboxCubeTexture = nullptr;
            SDL_GPUSampler* gSkyboxCubeSampler = nullptr;
            std::array<SDL_GPUTexture*, 6> gSkyboxFaceHandles {};
            int gSkyboxCubeSize = 0;
            SDL_GPUTexture* gDepthTexture = nullptr;
            SDL_GPUTextureFormat gDepthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
            int gDepthTextureWidth = 0;
            int gDepthTextureHeight = 0;
            Transform3D gCamera3DTransform {
                glm::vec3(0.0f, 0.0f, 3.0f),
                glm::vec3(0.0f),
                glm::vec3(1.0f)
            };
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
