#include "engine/renderers/software.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/core.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace CE::Renderers::Software {

    static SDL_Renderer* renderer = nullptr;
    static CE::Renderers::Colour drawColour{ 255, 255, 255, 255 };
    static CE::Renderers::Texture whiteTexture{ nullptr, 1, 1, CE::Renderers::TextureFormat::RGBA8 };

    static SDL_FRect ApplyCameraToRect(const CE::Renderers::RectF& r) {
        const auto& cam = CE::Renderers::camera;
        return SDL_FRect{
            (r.x - cam.x) * cam.zoom,
            (r.y - cam.y) * cam.zoom,
            r.w * cam.zoom,
            r.h * cam.zoom
        };
    }

    static SDL_FPoint ApplyCameraToOrigin(const CE::Renderers::Vec2& o) {
        const auto& cam = CE::Renderers::camera;
        return SDL_FPoint{ o.x * cam.zoom, o.y * cam.zoom };
    }

    static SDL_FlipMode ToSdlFlipMode(CE::Renderers::Flip flip) {
        switch (flip) {
            case CE::Renderers::Flip::Horizontal:
                return SDL_FLIP_HORIZONTAL;
            case CE::Renderers::Flip::Vertical:
                return SDL_FLIP_VERTICAL;
            case CE::Renderers::Flip::HorizontalAndVertical:
                return SDL_FLIP_HORIZONTAL_AND_VERTICAL;
            default:
                return SDL_FLIP_NONE;
        }
    }

    static bool ReadAllBytesFromVFS(const char* virtual_path, std::vector<uint8_t>& out_bytes) {
        out_bytes.clear();
        if (!virtual_path)
            return false;

        auto& vfs = CE::Global::GetVFS();
        uint64_t size64 = 0;
        if (!vfs.GetFileSize(virtual_path, size64))
            return false;

        if (size64 == 0) {
            out_bytes.clear();
            return true;
        }

        if (size64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            return false;

        auto* file = vfs.OpenFile(virtual_path);
        if (!file)
            return false;

        out_bytes.resize(static_cast<size_t>(size64));
        size_t offset = 0;
        while (offset < out_bytes.size()) {
            const size_t n = vfs.ReadFile(file, out_bytes.data() + offset, out_bytes.size() - offset);
            if (n == 0)
                break;
            offset += n;
        }
        vfs.CloseFile(file);

        if (offset != out_bytes.size())
            return false;

        return true;
    }

    void Init(SDL_Window* window) {
        renderer = SDL_CreateRenderer(window, "software");

        if (!renderer) {
            CE::Log(CE::LogLevel::Fatal, "[Software Renderer] Failed to create renderer");
            ShowError("[Software Renderer] Failed to create renderer");
            std::exit(1);
        }

        SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            CE::Log(CE::LogLevel::Fatal, "[Software Renderer] Failed to create white surface: {}", SDL_GetError());
            ShowError("[Software Renderer] Failed to create white surface");
            std::exit(1);
        }

        const Uint32 white = SDL_MapSurfaceRGBA(surface, 255, 255, 255, 255);
        SDL_FillSurfaceRect(surface, nullptr, white);

        SDL_Texture* whiteTex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!whiteTex) {
            CE::Log(CE::LogLevel::Fatal, "[Software Renderer] Failed to create white texture: {}", SDL_GetError());
            ShowError("[Software Renderer] Failed to create white texture");
            std::exit(1);
        }

        SDL_SetTextureBlendMode(whiteTex, SDL_BLENDMODE_BLEND);
        whiteTexture.handle = whiteTex;
    }

    void BeginFrame() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    void EndFrame() {
        SDL_RenderPresent(renderer);
    }

    std::shared_ptr<CE::Renderers::Texture> Load(const char* path) {
        std::vector<uint8_t> bytes;
        if (!ReadAllBytesFromVFS(path, bytes)) {
            CE::Log(CE::LogLevel::Error, "[Software renderer] Failed to read texture from VFS: {}", path ? path : "(null)");
            return nullptr;
        }

        SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
        if (!io) {
            CE::Log(CE::LogLevel::Error, "[Software renderer] Failed to create SDL_IOStream: {}", SDL_GetError());
            return nullptr;
        }

        SDL_Texture* tex = IMG_LoadTexture_IO(renderer, io, true);

        if (!tex) {
            CE::Log(CE::LogLevel::Error, "[Software renderer] Failed to create texture: {}", SDL_GetError());
            return nullptr;
        }

        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

        auto texture = std::make_shared<CE::Renderers::Texture>();
        texture->handle = tex;

        float w = 0.0f;
        float h = 0.0f;
        if (!SDL_GetTextureSize(tex, &w, &h)) {
            CE::Log(LogLevel::Error, "[Software renderer] Failed to query texture size: {}", SDL_GetError());
            texture->width = 0;
            texture->height = 0;
        } else {
            texture->width = static_cast<int>(w);
            texture->height = static_cast<int>(h);
        }
        texture->format = CE::Renderers::TextureFormat::RGBA8;

        return texture;
    }

    void Draw(CE::Renderers::Texture* texture, float x, float y, float w, float h) {
        const CE::Renderers::RectF dst{ x, y, w, h };
        CE::Renderers::DrawTextureOptions options;
        CE::Renderers::Software::Draw(texture, dst, options);
    }

    void Draw(CE::Renderers::Texture* texture, const CE::Renderers::RectF& dst, const CE::Renderers::DrawTextureOptions& options) {
        if (!renderer || !texture || !texture->handle)
            return;

        SDL_Texture* tex = static_cast<SDL_Texture*>(texture->handle);

        SDL_SetTextureColorMod(tex, options.tint.r, options.tint.g, options.tint.b);
        SDL_SetTextureAlphaMod(tex, options.tint.a);

        SDL_FRect dstRect = ApplyCameraToRect(dst);

        SDL_FRect srcRect;
        const SDL_FRect* srcPtr = nullptr;
        if (options.src.w > 0.0f && options.src.h > 0.0f) {
            srcRect.x = options.src.x;
            srcRect.y = options.src.y;
            srcRect.w = options.src.w;
            srcRect.h = options.src.h;
            srcPtr = &srcRect;
        }

        const SDL_FlipMode flip = ToSdlFlipMode(options.flip);

        if (options.rotation != 0.0f || flip != SDL_FLIP_NONE || options.origin.x != 0.0f || options.origin.y != 0.0f) {
            const SDL_FPoint center = ApplyCameraToOrigin(options.origin);
            SDL_RenderTextureRotated(renderer, tex, srcPtr, &dstRect, options.rotation, &center, flip);
        } else {
            SDL_RenderTexture(renderer, tex, srcPtr, &dstRect);
        }
    }

    void DrawRect(float x, float y, float w, float h) {
        if (w == 0.0f || h == 0.0f)
            return;
        if (w < 0.0f) {
            x += w;
            w = -w;
        }
        if (h < 0.0f) {
            y += h;
            h = -h;
        }

        const CE::Renderers::RectF dst{ x, y, w, h };
        CE::Renderers::DrawTextureOptions options;
        options.tint = drawColour;
        CE::Renderers::Software::Draw(&whiteTexture, dst, options);
    }

    void DrawRectLines(float x, float y, float w, float h, float thickness) {
        if (w == 0.0f || h == 0.0f || thickness <= 0.0f)
            return;
        if (w < 0.0f) {
            x += w;
            w = -w;
        }
        if (h < 0.0f) {
            y += h;
            h = -h;
        }

        thickness = std::min(thickness, std::min(w, h));
        const float innerH = std::max(0.0f, h - (2.0f * thickness));

        // Top
        DrawRect(x, y, w, thickness);
        // Bottom
        DrawRect(x, y + h - thickness, w, thickness);
        // Left
        DrawRect(x, y + thickness, thickness, innerH);
        // Right
        DrawRect(x + w - thickness, y + thickness, thickness, innerH);
    }

    void DrawLine(float x1, float y1, float x2, float y2, float thickness) {
        if (thickness <= 0.0f)
            return;

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float len = std::sqrt((dx * dx) + (dy * dy));
        if (len <= 0.0001f) {
            DrawRect(x1 - (thickness * 0.5f), y1 - (thickness * 0.5f), thickness, thickness);
            return;
        }

        constexpr float Pi = 3.14159265358979323846f;
        const float angleDeg = std::atan2(dy, dx) * (180.0f / Pi);

        CE::Renderers::RectF dst{ x1, y1 - (thickness * 0.5f), len, thickness };
        CE::Renderers::DrawTextureOptions options;
        options.rotation = angleDeg;
        options.origin = CE::Renderers::Vec2{ 0.0f, thickness * 0.5f };
        options.tint = drawColour;
        CE::Renderers::Software::Draw(&whiteTexture, dst, options);
    }

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        drawColour = CE::Renderers::Colour{ r, g, b, a };
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
    }

    void Shutdown() {
        if (whiteTexture.handle) {
            SDL_DestroyTexture(static_cast<SDL_Texture*>(whiteTexture.handle));
            whiteTexture.handle = nullptr;
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
    }
}
