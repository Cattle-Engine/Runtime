#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "engine/common/vfs.hpp"
namespace CE {
    enum class RendererBackend {
        Software,
        OpenGL,
        DX12,
        DX11,
        Metal,
        Vulkan,
        None
    };
}
namespace CE::Renderer {
    class IRenderer;

    struct GPUDevice {
        void* device;
        RendererBackend backend;
    };
    
    using GPUDeviceHandle = std::shared_ptr<GPUDevice>;
    
    inline RendererBackend renderer = RendererBackend::None;
    inline IRenderer* currentRenderer = nullptr;
    inline std::string rendererName = "None";

    IRenderer* CreateRenderer(RendererBackend backend, VFS::VFS* vfs);
    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo);
    void DestroyGPUDevice(GPUDeviceHandle device);
    
    struct Camera2D {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    enum class TextureFormat {
        RGBA8,
        RGB8,
        R8,
    };

    struct Colour { // BRITISH SPELLING HEEEEEEEEEHHEHEHE
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };

    struct Vertex {
        float x, y, z;
        uint8_t r, g, b, a;
        float u, v;
    };

    struct Texture {
        void* handle;    
        int width;
        int height;
        TextureFormat format;
        RendererBackend backend;
    };

    enum class TextureFilter {
        Nearest, 
        Linear  
    };

    enum class TextureWrap {
        Clamp,          // stretch edge pixels
        Repeat,         // tile the texture
        MirroredRepeat  // tile but mirrored each time
    };

    class IRenderer {
        public:
                virtual void PreWinInit() = 0;
            
                virtual int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) = 0;
                virtual int Shutdown(SDL_Window* window) = 0;

                virtual void ChangeCameraPos(float X, float Y, float zoom) = 0;
            
                virtual void DrawRect(float x, float y, float w, float h,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                    float rotation) = 0;
                virtual void DrawCircle(float cx, float cy, float radius,
                                        int segments,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
                virtual void DrawLine(float x1, float y1, float x2, float y2,
                                    float thickness,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
                virtual void SetClearColor(float r, float g, float b, float a) = 0;

                virtual Texture* LoadTex(const char* path) = 0;
                virtual Texture* CreateTextureFromData(
                    int width,
                    int height,
                    const void* pixels,
                    TextureFormat format,
                    int pitch = 0,
                    TextureFilter filter = TextureFilter::Linear,
                    TextureWrap wrap = TextureWrap::Clamp
                ) = 0;
                virtual void DrawTex(Texture* texture, float x, float y,
                                    float w, float h, Colour colour,
                                    float rotation) = 0;
                virtual void DrawTexUV(Texture* tex,
                    float x, float y,
                    float w, float h,
                    float u0, float v0,
                    float u1, float v1,
                    Colour colour,
                    float rotation) = 0;
                virtual void UnloadTex(Texture* texture) = 0;
                virtual void DrawTriangle(
                            float x0, float y0,
                            float x1, float y1,
                            float x2, float y2,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                            float rotation) = 0;
                virtual void DrawRectLines(float x, float y, float w, float h,
                                            float thickness,
                                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
                virtual void DrawCircleLines(float cx, float cy, float radius,
                                            int segments, float thickness,
                                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;

                virtual int BeginFrame(SDL_Window* window) = 0;
                virtual int EndFrame(SDL_Window* window) = 0;
               
                virtual Texture* GetErrorTexture() = 0;
                virtual void* GetNativeTextureHandle(Texture* texture) = 0;

                virtual int Debug_GetVertCount() = 0;
                virtual int Debug_GetIndexCount() = 0;
                virtual int Debug_GetTexIndexCount() = 0;
                virtual int Debug_GetTexVertCount() = 0;
                virtual Camera2D* GetCamera() = 0;

                virtual void SetVSync(bool setting) = 0;

                virtual void ImGuiStartFrame() = 0;
                virtual void ImGuiEndFrame(SDL_Window* window) = 0;

            virtual ~IRenderer() = default;
    };
}
