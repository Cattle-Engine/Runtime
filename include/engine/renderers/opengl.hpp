#pragma once

#include <SDL3/SDL.h>
#include <memory>

#include "engine/renderers.hpp"

namespace CE::Renderers::OpenGL {
    void Init(SDL_Window* window);
    void BeginFrame();
    void EndFrame();
    std::shared_ptr<CE::Renderers::Texture> Load(const char* path);
    void Draw(CE::Renderers::Texture* texture, float x, float y, float w, float h);
    void Draw(CE::Renderers::Texture* texture, const CE::Renderers::RectF& dst, const CE::Renderers::DrawTextureOptions& options);

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void DrawRect(float x, float y, float w, float h);
    void DrawRectLines(float x, float y, float w, float h, float thickness = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f);
    void Shutdown();
}
