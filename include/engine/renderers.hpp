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

namespace CE::Renderers {
    inline RendererBackend renderer = RendererBackend::None;
    inline std::string rendererName;
     
    struct Camera2D {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    inline Camera2D camera;

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

    enum class Flip {
        None = 0,
        Horizontal = 1,
        Vertical = 2,
        HorizontalAndVertical = 3,
    };

    struct Texture {
        void* handle;    
        int width;
        int height;
        TextureFormat format;
        RendererBackend backend;
    };

    struct DrawTextureOptions {
        // Source rect (in texture pixels). If src.w <= 0 or src.h <= 0, the whole texture is used.
        RectF src;

        // Rotation in degrees (clockwise, like SDL_RenderTextureRotated).
        float rotation = 0.0f;

        // Pivot (in destination pixels, relative to dst top-left).
        Vec2 origin;

        // Flipping.
        Flip flip = Flip::None;

        // Tint colour (multiplied with texture colour).
        Colour tint{ 255, 255, 255, 255 };
    };

    void Init(SDL_Window* window);
    void BeginFrame();
    void EndFrame();
    void ApplyCamera(int& x, int& y, int& size);
    std::shared_ptr<Texture> LoadTexture(const char* path);
    void Draw(Texture* texture, float x, float y, float w, float h);
    void Draw(Texture* texture, const RectF& dst, const DrawTextureOptions& options);

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    inline void SetColor(const Colour& c) { SetColor(c.r, c.g, c.b, c.a); }

    void DrawRect(float x, float y, float w, float h);
    void DrawRectLines(float x, float y, float w, float h, float thickness = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f);
    void DrawCircleLines(float cx, float cy, float radius, int segments = 32, float thickness = 1.0f);

    void Shutdown();
}
