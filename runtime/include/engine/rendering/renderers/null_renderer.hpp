
#pragma once

#include "engine/rendering/renderer.hpp"

namespace CE::Renderer {
    class NullRenderer final : public IRenderer {
    public:
        void PreWinInit() override {
        }

        int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override {
            (void)window;
            (void)debug;
            (void)gdevice;
            return 0;
        }

        int Shutdown(SDL_Window* window) override {
            (void)window;
            return 0;
        }

        void ChangeCameraPos2D(float X, float Y, float zoom) override {
            mCamera2D.x = X;
            mCamera2D.y = Y;
            mCamera2D.zoom = zoom;
        }

        void ChangeCameraPos3D(const Transform3D& transform) override {
            mCamera3DState.position = transform.position;
            mCamera3DState.rotation = transform.rotation;
        }

        void BeginMode2D() override {
        }

        void EndMode2D() override {
        }

        void BeginMode3D() override {
        }

        void EndMode3D() override {
        }

        void SetClearColour(float r, float g, float b, float a) override {
            (void)r;
            (void)g;
            (void)b;
            (void)a;
        }

        Texture* LoadTex(const char* path) override {
            (void)path;
            return nullptr;
        }

        Texture* CreateTextureFromData(
            int width,
            int height,
            const void* pixels,
            TextureFormat format,
            int pitch = 0,
            TextureFilter filter = TextureFilter::Linear,
            TextureWrap wrap = TextureWrap::Clamp,
            TextureUploadBatch* batch = nullptr
        ) override {
            (void)width;
            (void)height;
            (void)pixels;
            (void)format;
            (void)pitch;
            (void)filter;
            (void)wrap;
            (void)batch;
            return nullptr;
        }

        void DrawSprite(
            Texture* texture,
            float x,
            float y,
            float w,
            float h,
            Colour colour,
            float rotation,
            TextureFlip flip = TextureFlip::None
        ) override {
            (void)texture;
            (void)x;
            (void)y;
            (void)w;
            (void)h;
            (void)colour;
            (void)rotation;
            (void)flip;
        }

        void DrawSpriteUV(
            Texture* tex,
            float x,
            float y,
            float w,
            float h,
            float u0,
            float v0,
            float u1,
            float v1,
            Colour colour,
            float rotation,
            TextureFlip flip = TextureFlip::None
        ) override {
            (void)tex;
            (void)x;
            (void)y;
            (void)w;
            (void)h;
            (void)u0;
            (void)v0;
            (void)u1;
            (void)v1;
            (void)colour;
            (void)rotation;
            (void)flip;
        }

        void UnloadTex(Texture* texture) override {
            (void)texture;
        }

        void DrawTriangle(
            float x0, float y0,
            float x1, float y1,
            float x2, float y2,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a,
            float rotation
        ) override {
            (void)x0;
            (void)y0;
            (void)x1;
            (void)y1;
            (void)x2;
            (void)y2;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
            (void)rotation;
        }

        void DrawRectLines(
            float x, float y,
            float w, float h,
            float thickness,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a
        ) override {
            (void)x;
            (void)y;
            (void)w;
            (void)h;
            (void)thickness;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
        }

        void DrawCircleLines(
            float cx, float cy,
            float radius,
            int segments,
            float thickness,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a
        ) override {
            (void)cx;
            (void)cy;
            (void)radius;
            (void)segments;
            (void)thickness;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
        }

        void DrawRect(
            float x, float y,
            float w, float h,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a,
            float rotation
        ) override {
            (void)x;
            (void)y;
            (void)w;
            (void)h;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
            (void)rotation;
        }

        void DrawCircle(
            float cx, float cy,
            float radius,
            int segments,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a
        ) override {
            (void)cx;
            (void)cy;
            (void)radius;
            (void)segments;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
        }

        void DrawLine(
            float x1, float y1,
            float x2, float y2,
            float thickness,
            uint8_t r, uint8_t g, uint8_t b, uint8_t a
        ) override {
            (void)x1;
            (void)y1;
            (void)x2;
            (void)y2;
            (void)thickness;
            (void)r;
            (void)g;
            (void)b;
            (void)a;
        }

        int BeginFrame(SDL_Window* window) override {
            (void)window;
            return 0;
        }

        int EndFrame(SDL_Window* window) override {
            (void)window;
            return 0;
        }

        Texture* GetErrorTexture() override {
            return nullptr;
        }

        Texture* GetWhiteTexture() override {
            return nullptr;
        }

        void* GetNativeTextureHandle(Texture* texture) override {
            (void)texture;
            return nullptr;
        }

        int Debug_GetVertCount() override {
            return 0;
        }

        int Debug_GetIndexCount() override {
            return 0;
        }

        int Debug_GetTexIndexCount() override {
            return 0;
        }

        int Debug_GetTexVertCount() override {
            return 0;
        }

        Camera2D* GetCamera() override {
            return &mCamera2D;
        }

        void SetVSync(bool setting) override {
            (void)setting;
        }

        Shader* CreateShaderProgram() override {
            return nullptr;
        }

        Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override {
            (void)path;
            (void)fragmentSamplerCount;
            return nullptr;
        }

        bool LoadShaderStage(
            Shader* shaderProgram,
            const char* path,
            ShaderStage stage,
            int samplerCount = 1
        ) override {
            (void)shaderProgram;
            (void)path;
            (void)stage;
            (void)samplerCount;
            return false;
        }

        bool UseDefaultShaderStage(Shader* shaderProgram, ShaderStage stage) override {
            (void)shaderProgram;
            (void)stage;
            return false;
        }

        bool CompileShaderProgram(Shader* shaderProgram) override {
            (void)shaderProgram;
            return false;
        }

        void UnloadShader(Shader* shader) override {
            (void)shader;
        }

        void BindShader(Shader* shader) override {
            (void)shader;
        }

        void UnbindShader() override {
        }

        void SetShaderFloat(const char* name, float value) override {
            (void)name;
            (void)value;
        }

        void SetShaderVec2(const char* name, float x, float y) override {
            (void)name;
            (void)x;
            (void)y;
        }

        void SetShaderVec3(const char* name, float x, float y, float z) override {
            (void)name;
            (void)x;
            (void)y;
            (void)z;
        }

        void SetShaderVec4(
            const char* name,
            float x, float y, float z, float w
        ) override {
            (void)name;
            (void)x;
            (void)y;
            (void)z;
            (void)w;
        }

        void SetShaderMat4(const char* name, const float* mat4) override {
            (void)name;
            (void)mat4;
        }

        void SetShaderInt(const char* name, int value) override {
            (void)name;
            (void)value;
        }

        void SetShaderTexture(const char* name, Texture* texture, int slot) override {
            (void)name;
            (void)texture;
            (void)slot;
        }

        GPUMesh* CreateGPUMesh(MeshData& mesh) override {
            (void)mesh;
            return nullptr;
        }

        void DestroyGPUMesh(GPUMesh* mesh) override {
            (void)mesh;
        }

        void DrawMesh(
            GPUMesh* mesh,
            Material& material,
            const Transform3D& transform,
            bool error_tex
        ) override {
            (void)mesh;
            (void)material;
            (void)transform;
            (void)error_tex;
        }

        void DrawMeshMat4(
            GPUMesh* mesh,
            Material& material,
            const glm::mat4& transform,
            bool error_tex
        ) override {
            (void)mesh;
            (void)material;
            (void)transform;
            (void)error_tex;
        }

        TextureUploadBatch* BeginBatchTextureUpload() override {
            return nullptr;
        }

        void EndBatchTextureUpload(TextureUploadBatch* batch) override {
            (void)batch;
        }

        void ImGuiStartFrame() override {
        }

        void ImGuiEndFrame(SDL_Window* window) override {
            (void)window;
        }

    private:
        Camera2D mCamera2D{};
    };
}