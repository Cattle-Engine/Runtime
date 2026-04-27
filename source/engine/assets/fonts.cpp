#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL.h>

#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/assets/default_font.hpp"
#include "engine/common/vfs.hpp"
#include "engine/assets/fonts.hpp"

namespace CE::Assets::Fonts {
    namespace {
        constexpr int ATLAS_W = 2048;
        constexpr int ATLAS_H = 2048;
        const std::string kFallbackAtlasName = "__ce_internal_fallback";

        bool IsFallbackFont(TTF_Font* font, const std::unordered_map<int, TTF_Font*>& fallbackFonts) {
            for (auto& [_, candidate] : fallbackFonts)
                if (candidate == font)
                    return true;
            return false;
        }
    }

    FontManager::FontManager(Renderer::IRenderer& renderer, VFS::VFS& vfs, uint64_t instance_id)
        : mVFS(vfs),
          mRenderer(renderer)
    {
        mInstanceID = instance_id;
        TTF_Init();
        mDefaultFontName.clear();
        GetFallbackFont(24);
        BuildAtlas(kFallbackAtlasName, mFallbackFonts[24], 24);
        mDefaultFontName = kFallbackAtlasName;
    }

    void FontManager::Update() {
        for (auto& [_, atlas] : mAtlases) {
            UpdateAtlasTexture(atlas);
        }
    }

    FontManager::~FontManager() {
        std::unordered_set<TTF_Font*> closedFonts;

        for (auto& [name, atlas] : mAtlases) {
            if (atlas.texture)
                mRenderer.UnloadTex(atlas.texture);
            if (atlas.atlasSurface)
                SDL_DestroySurface(atlas.atlasSurface);
            if (atlas.font && !closedFonts.count(atlas.font)) {
                TTF_CloseFont(atlas.font);
                closedFonts.insert(atlas.font);
            }
        }
        mAtlases.clear();

        for (auto& [_, font] : mFallbackFonts) {
            if (font && !closedFonts.count(font)) {
                TTF_CloseFont(font);
                closedFonts.insert(font);
            }
        }
        mFallbackFonts.clear();
        TTF_Quit();
    }

    bool FontManager::Load(const std::string& path, const std::string& name, int size) {
        if (name.empty() || path.empty())
            return false;

        if (mAtlases.find(name) != mAtlases.end())
            Unload(name);

        TTF_Font* font = LoadFontFromVFS(path, size);
        if (!font) {
            CE::Log(LogLevel::Error,
                    "[Font Manager {}] Failed to load font: {}",
                    mInstanceID, path);
            return false;
        }

        BuildAtlas(name, font, size);
        if (mDefaultFontName.empty() || mDefaultFontName == kFallbackAtlasName)
            mDefaultFontName = name;
        return true;
    }

    void FontManager::SetDefault(const std::string& name) {
        if (mAtlases.find(name) != mAtlases.end()) {
            mDefaultFontName = name;
        } else {
            CE::Log(LogLevel::Warn,
                    "[Font Manager {}] Cannot set default font '{}': not loaded",
                    mInstanceID, name);
        }
    }

    void FontManager::Unload(const std::string& name) {
        auto it = mAtlases.find(name);
        if (it == mAtlases.end())
            return;

        FontAtlas& atlas = it->second;
        if (atlas.texture)
            mRenderer.UnloadTex(atlas.texture);
        if (atlas.atlasSurface)
            SDL_DestroySurface(atlas.atlasSurface);
        if (atlas.font && !IsFallbackFont(atlas.font, mFallbackFonts))
            TTF_CloseFont(atlas.font);

        mAtlases.erase(it);
        if (mDefaultFontName == name)
            mDefaultFontName = kFallbackAtlasName;
    }

    void FontManager::UnloadAll() {
        for (auto& [name, atlas] : mAtlases) {
            if (atlas.texture)
                mRenderer.UnloadTex(atlas.texture);
            if (atlas.atlasSurface)
                SDL_DestroySurface(atlas.atlasSurface);
            if (atlas.font && !IsFallbackFont(atlas.font, mFallbackFonts))
                TTF_CloseFont(atlas.font);
        }
        mAtlases.clear();
        mDefaultFontName.clear();
    }

    void FontManager::Draw(const std::string& text, int x, int y, float size, Renderer::Colour colour) {
        DrawEx(text, mDefaultFontName, x, y, size, colour);
    }

    void FontManager::DrawEx(const std::string& text,
                            const std::string& name,
                            int x, int y,
                            float size,
                            Renderer::Colour colour)
    {
        auto it = mAtlases.find(name);
        if (it == mAtlases.end())
            return;

        FontAtlas& atlas = it->second;

        float scale = (size > 0.0f ? (size / static_cast<float>(atlas.fontSize)) : 1.0f);
        float cursorX = (float)x;
        const int lineHeight = static_cast<int>(TTF_GetFontHeight(atlas.font) * scale);
        size_t i = 0;
        uint32_t cp = 0;
        uint32_t prevCp = 0;
        bool hasKerning = TTF_GetFontKerning(atlas.font) != 0;

        while (i < text.size())
        {
            if (!DecodeUTF8(text, cp, i))
                cp = '?';

            if (cp == u'\n') {
                cursorX = (float)x;
                y += lineHeight;
                prevCp = 0;
                continue;
            }

            if (!EnsureGlyph(atlas, cp))
                continue;

            const Glyph& g = atlas.glyphs[cp];

            if (hasKerning && prevCp != 0) {
                const Glyph& prevG = atlas.glyphs[prevCp];
                if (g.font == prevG.font) {
                    KerningKey key{prevCp, cp};
                    auto cacheIt = atlas.kerningCache.find(key);
                    int kerning = 0;
                    if (cacheIt != atlas.kerningCache.end()) {
                        kerning = cacheIt->second;
                    } else {
                        if (TTF_GetGlyphKerning(g.font, prevCp, cp, &kerning)) {
                            atlas.kerningCache[key] = kerning;
                        } else {
                            kerning = 0;
                        }
                    }
                    cursorX += kerning * 2 * scale;
                }
            }

            float drawX = cursorX + g.bearingX * scale;
            float drawY = (float)y + (TTF_GetFontAscent(atlas.font) - g.bearingY) * scale;

            mRenderer.DrawTexUV(
                atlas.texture,
                drawX, drawY,
                (float)g.w * scale,
                (float)g.h * scale,
                g.u0, g.v0,
                g.u1, g.v1,
                colour,
                0.0f
            );

            cursorX += (float)g.advance * scale;
            prevCp = cp;
        }
    }

    void FontManager::BuildAtlas(const std::string& name, TTF_Font* font, int fontSize) {
        FontAtlas atlas;
        atlas.font = font;
        atlas.fontSize = fontSize;
        atlas.penX = 0;
        atlas.penY = 0;
        atlas.rowH = 0;
        atlas.atlasSurface = SDL_CreateSurface(ATLAS_W, ATLAS_H, SDL_PIXELFORMAT_RGBA32);
        if (!atlas.atlasSurface) {
            CE::Log(LogLevel::Error,
                    "[Font Manager {}] Failed to create font atlas surface",
                    mInstanceID);
            return;
        }

        SDL_FillSurfaceRect(atlas.atlasSurface, nullptr, 0x00000000);
        UpdateAtlasTexture(atlas);
        mAtlases[name] = std::move(atlas);
    }

    void FontManager::UpdateAtlasTexture(FontAtlas& atlas) {
        if (!atlas.atlasSurface)
            return;

        if (!atlas.dirty)
            return;

        Renderer::Texture* newTex =
            mRenderer.CreateTextureFromData(
                atlas.atlasSurface->w,
                atlas.atlasSurface->h,
                atlas.atlasSurface->pixels,
                Renderer::TextureFormat::RGBA8,
                atlas.atlasSurface->pitch
            );

        if (atlas.texture)
            mRenderer.UnloadTex(atlas.texture);

        atlas.texture = newTex;
        atlas.dirty = false;
    }

    TTF_Font* FontManager::LoadFontFromVFS(const std::string& path, int pointSize) {
        VirtualFile* file = mVFS.OpenFile(path.c_str());
        if (!file || !file->sdl_stream)
            return nullptr;

        SDL_IOStream* stream = file->sdl_stream;
        TTF_Font* font = TTF_OpenFontIO(stream, 1, pointSize);
        if (!font) {
            SDL_CloseIO(stream);
        }
        mVFS.CloseFile(file);
        return font;
    }

    TTF_Font* FontManager::GetFallbackFont(int pointSize) {
        auto it = mFallbackFonts.find(pointSize);
        if (it != mFallbackFonts.end())
            return it->second;

        SDL_IOStream* stream = SDL_IOFromMem(Default::default_ce_font, Default::default_ce_font_len);
        if (!stream)
            return nullptr;

        TTF_Font* font = TTF_OpenFontIO(stream, 1, pointSize);
        if (!font) {
            SDL_CloseIO(stream);
        }
        if (!font)
            return nullptr;

        mFallbackFonts[pointSize] = font;
        return font;
    }

    bool FontManager::EnsureGlyph(FontAtlas& atlas, uint32_t codepoint) {
        if (atlas.glyphs.contains(codepoint))
            return true;

        SDL_Color white = {255, 255, 255, 255};

        TTF_Font* font = atlas.font;

        SDL_Surface* glyphSurface =
            TTF_RenderGlyph_Blended(font, static_cast<int>(codepoint), white);

        if (!glyphSurface)
        {
            font = GetFallbackFont(atlas.fontSize);
            if (!font) return false;

            glyphSurface =
                TTF_RenderGlyph_Blended(font, static_cast<int>(codepoint), white);

            if (!glyphSurface) return false;
        }

        // Scale glyph to 2x for better quality when scaled up
        SDL_Surface* scaledSurface = SDL_CreateSurface(glyphSurface->w * 2, glyphSurface->h * 2, SDL_PIXELFORMAT_RGBA32);
        if (scaledSurface) {
            SDL_BlitSurfaceScaled(glyphSurface, nullptr, scaledSurface, nullptr, SDL_SCALEMODE_LINEAR);
            SDL_DestroySurface(glyphSurface);
            glyphSurface = scaledSurface;
        } else {
            // If scaling fails, use original
            CE::Log(LogLevel::Warn, "[Font Manager {}] Failed to scale glyph surface, using original", mInstanceID);
        }

        int minx, maxx, miny, maxy, advance;
        if (!TTF_GetGlyphMetrics(font, codepoint,
                                &minx, &maxx, &miny, &maxy, &advance))
        {
            advance = glyphSurface->w;
            minx = 0;
            maxy = glyphSurface->h;
        }

        // Scale metrics by 2
        advance *= 2;
        minx *= 2;
        maxx *= 2;
        miny *= 2;
        maxy *= 2;

        const int padding = 1;

        if (atlas.penX + glyphSurface->w + padding >= ATLAS_W)
        {
            atlas.penX = 0;
            atlas.penY += atlas.rowH + padding;
            atlas.rowH = 0;
        }

        if (atlas.penY + glyphSurface->h >= ATLAS_H)
        {
            SDL_DestroySurface(glyphSurface);
            return false;
        }

        SDL_Rect dst = {
            atlas.penX,
            atlas.penY,
            glyphSurface->w,
            glyphSurface->h
        };

        SDL_BlitSurface(glyphSurface, nullptr, atlas.atlasSurface, &dst);

        Glyph g;
        g.u0 = atlas.penX / (float)ATLAS_W;
        g.v0 = atlas.penY / (float)ATLAS_H;
        g.u1 = (atlas.penX + glyphSurface->w) / (float)ATLAS_W;
        g.v1 = (atlas.penY + glyphSurface->h) / (float)ATLAS_H;

        g.w = glyphSurface->w;
        g.h = glyphSurface->h;

        g.advance = advance;
        g.bearingX = minx;
        g.bearingY = maxy;
        g.font = font;

        atlas.glyphs[codepoint] = g;

        atlas.penX += glyphSurface->w + padding;
        atlas.rowH = std::max(atlas.rowH, glyphSurface->h);

        SDL_DestroySurface(glyphSurface);

        atlas.dirty = true;

        return true;
    }

    bool FontManager::DecodeUTF8(const std::string& text, uint32_t& outCodepoint, size_t& cursor) const {
        if (cursor >= text.size())
            return false;

        unsigned char first = static_cast<unsigned char>(text[cursor]);
        if (first < 0x80) {
            outCodepoint = first;
            cursor += 1;
            return true;
        }

        int expected = 0;
        uint32_t codepoint = 0;

        if ((first & 0xE0) == 0xC0) {
            expected = 2;
            codepoint = first & 0x1F;
        } else if ((first & 0xF0) == 0xE0) {
            expected = 3;
            codepoint = first & 0x0F;
        } else if ((first & 0xF8) == 0xF0) {
            expected = 4;
            codepoint = first & 0x07;
        } else {
            cursor += 1;
            outCodepoint = 0xFFFD;
            return false;
        }

        if (cursor + expected > text.size()) {
            cursor += 1;
            outCodepoint = 0xFFFD;
            return false;
        }

        for (int i = 1; i < expected; ++i) {
            unsigned char ch = static_cast<unsigned char>(text[cursor + i]);
            if ((ch & 0xC0) != 0x80) {
                cursor += 1;
                outCodepoint = 0xFFFD;
                return false;
            }
            codepoint = (codepoint << 6) | (ch & 0x3F);
        }

        cursor += expected;
        outCodepoint = codepoint;
        return true;
    }
}