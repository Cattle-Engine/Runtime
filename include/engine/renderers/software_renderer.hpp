#pragma once

#include <cstdint>
#include <unordered_set>

#include <SDL3/SDL.h>

#include "engine/renderer.hpp"
#include "engine/common/fs/vfs.hpp"

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

            void ChangeCameraPos(float X, float Y, float zoom) override;

            void DrawRect(float x, float y, float w, float h,
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
            void DrawTriangle(float x0, float y0,
                              float x1, float y1,
                              float x2, float y2,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                              float rotation) override;
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
            struct SoftwareTextureData {
                SDL_Texture* texture = nullptr;
            };

            SDL_FPoint ApplyCamera(float x, float y) const;
            bool SetDrawColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
            bool RenderTexture(Texture* texture,
                               float x, float y,
                               float w, float h,
                               const SDL_FRect* srcRect,
                               Colour colour,
                               float rotation);
            Texture* CreateTextureFromSurface(SDL_Surface* surface);
            void DestroyTexture(Texture* texture);
            void EnsureImGuiContext();
            bool CreateImGuiFontTexture();
            void DestroyImGuiFontTexture();
            void ImGuiInit(SDL_Window* window);
            void ImGuiShutdown();
            void RenderImGuiDrawData(ImDrawData* drawData);

            SDL_Renderer* mRenderer = nullptr;
            Camera2D mCamera{};
            SDL_Color mClearColor{0, 0, 0, 255};
            Texture* mWhiteTexture = nullptr;
            Texture* mErrorTexture = nullptr;
            VFS::VFS* mVFS = nullptr;
            bool mVSyncEnabled = true;
            void* mImGuiContext = nullptr;
            Texture* mImGuiFontTexture = nullptr;
            std::unordered_set<Texture*> mOwnedTextures;
    };
}
