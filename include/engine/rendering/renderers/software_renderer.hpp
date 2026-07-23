#pragma once

#include <cstdint>
#include <memory>
#include <unordered_set>

#include <SDL3/SDL.h>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

struct ImGuiContext;
struct ImDrawData;

namespace CE::Renderer::Software {
    GPUDeviceHandle CreateGPUDevice();

    class Software_Renderer : public Renderer::IRenderer {
      public:
        Software_Renderer(VFS::VFS* vfs);
        ~Software_Renderer() override;

        void PreWinInit() override;

        int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;
        int Shutdown(SDL_Window* window) override;

        void ChangeCameraPos2D(float X, float Y, float zoom) override;

        void DrawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                      float rotation) override;
        void DrawCircle(float cx, float cy, float radius, int segments, uint8_t r, uint8_t g, uint8_t b,
                        uint8_t a) override;
        void DrawLine(float x1, float y1, float x2, float y2, float thickness, uint8_t r, uint8_t g, uint8_t b,
                      uint8_t a) override;
        void SetClearColor(float r, float g, float b, float a) override;

        Texture* LoadTex(const char* path) override;
        Texture* CreateTextureFromData(int width, int height, const void* pixels, TextureFormat format, int pitch = 0,
                                       TextureFilter filter = TextureFilter::Linear,
                                       TextureWrap wrap = TextureWrap::Clamp,
                                       TextureUploadBatch* batch = nullptr) override;
        void DrawTex(Texture* texture, float x, float y, float w, float h, Colour colour, float rotation,
                     TextureFlip flip = TextureFlip::None) override;
        void DrawTexUV(Texture* tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                       Colour colour, float rotation, TextureFlip flip = TextureFlip::None) override;
        void UnloadTex(Texture* texture) override;
        void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a, float rotation) override;
        void DrawRectLines(float x, float y, float w, float h, float thickness, uint8_t r, uint8_t g, uint8_t b,
                           uint8_t a) override;
        void DrawCircleLines(float cx, float cy, float radius, int segments, float thickness, uint8_t r, uint8_t g,
                             uint8_t b, uint8_t a) override;

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
        void BeginMode3D() override;
        void EndMode3D() override;
        void ChangeCameraPos3D(const Transform3D& transform) override;

        void DrawMeshMat4(GPUMesh* mesh, Material& material, const glm::mat4& transform, bool error_tex) override;

        void BeginMode2D() override;
        void EndMode2D() override;

        void ImGuiStartFrame() override;
        void ImGuiEndFrame(SDL_Window* window) override;

      private:
        struct SoftwareTextureData {
            SDL_Texture* texture = nullptr;
        };

        SDL_FPoint ApplyCamera(float x, float y) const;
        bool SetDrawColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        bool RenderTexture(Texture* texture, float x, float y, float w, float h, const SDL_FRect* srcRect,
                           Colour colour, float rotation, SDL_FlipMode flipMode);
        Texture* CreateTextureFromSurface(SDL_Surface* surface);
        void DestroyTexture(Texture* texture);
        void EnsureImGuiContext();
        bool CreateImGuiFontTexture();
        void DestroyImGuiFontTexture();
        void ImGuiInit(SDL_Window* window);
        void ImGuiShutdown();
        void RenderImGuiDrawData(ImDrawData* drawData);
        static void LogAboutShaders();
        static void LogAbout3D();

        SDL_Renderer* mRenderer = nullptr;
        Camera2D mCamera{};
        SDL_Color mClearColor{0, 0, 0, 255};
        Texture* mWhiteTexture = nullptr;
        Texture* mErrorTexture = nullptr;
        VFS::VFS* mVFS = nullptr;
        bool mVSyncEnabled = true;
        bool m2DFrameActive = false; // This is here to enforce API usage game side
        bool mFrameActive = false;   // This as well
        void* mImGuiContext = nullptr;
        Texture* mImGuiFontTexture = nullptr;
        std::unordered_set<Texture*> mOwnedTextures;
    };

    struct Software_Shader {};
} // namespace CE::Renderer::Software
