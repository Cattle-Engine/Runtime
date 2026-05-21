#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/tracelog.hpp"

#include <cmath>

namespace CE::Assets::Textures {
    constexpr int ERROR_SIZE = 50;
    constexpr float MIN_TILE_COUNT = 0.0001f;

    static CE::Renderer::TextureFlip ToTextureFlip(bool flipX, bool flipY) {
        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX) {
            flip = flip | CE::Renderer::TextureFlip::Horizontal;
        }
        if (flipY) {
            flip = flip | CE::Renderer::TextureFlip::Vertical;
        }
        return flip;
    }

    static float SanitizeTileCount(float tileCount) {
        return tileCount > 0.0f ? tileCount : 1.0f;
    }

    TextureManager::TextureManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
        gErrorTex = gRenderer->GetErrorTexture();
    }

    static void DrawTextureWithOptions(CE::Renderer::IRenderer* renderer,
                                       CE::Renderer::Texture* texture,
                                       int x, int y,
                                       int w, int h,
                                       float rotation,
                                       CE::Renderer::Colour colour,
                                       bool flipX, bool flipY,
                                       float tileX, float tileY) {
        const float resolvedTileX = SanitizeTileCount(tileX);
        const float resolvedTileY = SanitizeTileCount(tileY);
        const bool isTiled =
            std::abs(resolvedTileX - 1.0f) > MIN_TILE_COUNT ||
            std::abs(resolvedTileY - 1.0f) > MIN_TILE_COUNT;

        const float left = static_cast<float>(x - w / 2);
        const float top = static_cast<float>(y - h / 2);

        if (!isTiled) {
            renderer->DrawTex(
                texture,
                left,
                top,
                static_cast<float>(w),
                static_cast<float>(h),
                colour,
                rotation,
                ToTextureFlip(flipX, flipY)
            );
            return;
        }

        const float u0 = flipX ? resolvedTileX : 0.0f;
        const float v0 = flipY ? resolvedTileY : 0.0f;
        const float u1 = flipX ? 0.0f : resolvedTileX;
        const float v1 = flipY ? 0.0f : resolvedTileY;

        renderer->DrawTexUV(
            texture,
            left,
            top,
            static_cast<float>(w),
            static_cast<float>(h),
            u0, v0,
            u1, v1,
            colour,
            rotation
        );
    }

    void TextureManager::Load(const char* filepath, const char* name) {
        CE::Renderer::Texture* tex;
        TMTexture texinfo = {};
        if (!gVFS->FileExists(filepath)) {
            CE::Log(LogLevel::Error, "[Texture Manager] Missing image: {}", filepath);
            tex = gRenderer->GetErrorTexture();
            texinfo.IsErrorTex = true;
            texinfo.Path = filepath;
        } else {
            tex = gRenderer->LoadTex(filepath);
            texinfo.Path = filepath;
        }
        texinfo.Texture = tex;
        gTextures[name] = texinfo;
    }

    void TextureManager::DrawPro(const char* name, int x, int y, int w,
        int h, float rotation, CE::Renderer::Colour colour, bool flipX, bool flipY,
        float tileX, float tileY) {
        auto tex = gTextures.find(name);
        if (tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                DrawTextureWithOptions(gRenderer, tex->second.Texture, x, y, w, h, rotation, colour, flipX, flipY, tileX, tileY);
                return;
            } else {
                if (!tex->second.ShownMissingError) {
                    CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw a missing asset: {}", tex->second.Path);
                    tex->second.ShownMissingError = true;
                }
                DrawTextureWithOptions(gRenderer, tex->second.Texture, x, y, w, h, 0.0f, {255, 255, 255, 255}, flipX, flipY, tileX, tileY);
                return;
            }
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw an unloaded or missing asset: {}", name);
        DrawTextureWithOptions(gRenderer, gErrorTex, x, y, w, h, 0.0f, {255, 255, 255, 255}, flipX, flipY, tileX, tileY);
    }

    void TextureManager::Unload(const char* name) {
        auto tex = gTextures.find(name);
        if(tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                gRenderer->UnloadTex(tex->second.Texture);
                CE::Log(LogLevel::Info, "[Texture Manager] Unloaded texture {}", name);
                return;
            } else {
                CE::Log(LogLevel::Warn, "[Texture Manager] Can not unload error texture! \n                 Deleting from texture list");
                return;
            }
            gTextures.erase(name);
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Can not unload a non-existant texture");
    }

    void TextureManager::DrawRot(const char* name, int x, int y,
            float rotation, CE::Renderer::Colour colour, bool flipX, bool flipY,
            float tileX, float tileY) {
        auto tex = gTextures.find(name);
        if (tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                int w = tex->second.Texture->width;
                int h = tex->second.Texture->height;
                DrawTextureWithOptions(gRenderer, tex->second.Texture, x, y, w, h, rotation, colour, flipX, flipY, tileX, tileY);
                return;
            } else {
                if (!tex->second.ShownMissingError) {
                    CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw a missing asset: {}", tex->second.Path);
                    tex->second.ShownMissingError = true;
                }
                DrawTextureWithOptions(gRenderer, tex->second.Texture, x, y, ERROR_SIZE, ERROR_SIZE, rotation, {255, 255, 255, 255}, flipX, flipY, tileX, tileY);
                return; 
            }
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw an unloaded or missing asset: {}", name);
        DrawTextureWithOptions(gRenderer, gErrorTex, x, y, ERROR_SIZE, ERROR_SIZE, rotation, {255, 255, 255, 255}, flipX, flipY, tileX, tileY);
    }

    void TextureManager::Draw(const char* name, int x, int y, CE::Renderer::Colour colour, bool flipX, bool flipY, float tileX, float tileY) {
        TextureManager::DrawRot(name, x, y, 0.0f, colour, flipX, flipY, tileX, tileY);
    }

    void TextureManager::UnloadAll() {
        for (auto& [name, texinfo] : gTextures) {
            if (!texinfo.IsErrorTex) {
                gRenderer->UnloadTex(texinfo.Texture);
            }
        }
        gTextures.clear();
        CE::Log(LogLevel::Info, "[Texture Manager] Unloaded all textures");
    }

    CE::Renderer::Texture* TextureManager::Get(const char* name) {
        auto tex = gTextures.find(name ? name : "");
        if (tex == gTextures.end()) {
            return nullptr;
        }
        return tex->second.Texture;
    }

    int TextureManager::Debug_LoadedTexturesCount()  {
        return gTextures.size();
    }

    int TextureManager::Debug_LoadedTexturesNoError() {
        int count = 0;
            for (auto& [name, tex] : gTextures) {
                if (!tex.IsErrorTex) count++;
            }
        return count;
    }

    int TextureManager::Debug_LoadedTexturesError() {
        int count = 0;
            for (auto& [name, tex] : gTextures) {
                if (tex.IsErrorTex) count++;
            }
        return count; 
    }

    TextureManager::~TextureManager() {
        UnloadAll();
        gErrorTex = nullptr;
    }
}
