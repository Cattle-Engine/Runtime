#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <SDL3/SDL.h>

enum class RendererBackend {
    Software,
    OpenGL,
    DirectX,
    Metal,
    Vulkan,
    None
};

namespace CE::Renderer {
    class IRenderer;

    inline RendererBackend renderer = RendererBackend::None;
    inline IRenderer* currentRenderer = nullptr;
    inline std::string rendererName = "None";

    IRenderer* CreateRenderer(RendererBackend backend);
    struct Camera2D {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    enum class TextureFormat {
        RGBA8,
        RGB8,
    };

    struct Colour { // BRITISH SPELLING HEEEEEEEEEHHEHEHE
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct RectF {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct Texture {
        void* handle;    
        int width;
        int height;
        TextureFormat format;
        RendererBackend backend;
    };

    class IRenderer {
        public:
            virtual void PreWinInit() = 0;
            virtual void Init(SDL_Window* window) = 0;
            virtual void Shutdown() = 0;

            virtual void BeginFrame(SDL_Window* window) = 0;
            virtual void EndFrame(SDL_Window* window) = 0;

           // virtual Texture* Load(const char* path) = 0;
           // virtual void Draw(Texture* texture, float x, float y, float w, float h, Colour colour) = 0;
           //virtual void DrawRect(float x, float y, float w, float h) = 0;
           // virtual void DrawRectLines(float x, float y, float w, float h, float thickness = 1.0f) = 0;
           //virtual void DrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f) = 0;
            //virtual void DrawCircleLines(float cx, float cy, float radius, int segments = 32, float thickness = 1.0f) = 0;

            virtual ~IRenderer() = default;
    };
}
