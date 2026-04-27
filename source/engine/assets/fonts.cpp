#include <algorithm>
#include <cmath>
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

        const std::string kFallbackFamilyName = "__ce_internal_fallback";
    }

    FontManager::FontManager(Renderer::IRenderer& renderer, VFS::VFS& vfs, uint64_t instance_id)
        : mVFS(vfs), mRenderer(renderer)
    {
        mInstanceID = instance_id;
        TTF_Init();

        mFamilies[kFallbackFamilyName] = {"", true};
        GetOrCreateAtlas(kFallbackFamilyName, 24);
        mDefaultFontName = kFallbackFamilyName;
    }

    FontManager::~FontManager() {
        std::unordered_set<TTF_Font*> closed;

        auto closeFont = [&](TTF_Font* font) {
            if (!font || closed.count(font))
                return;

            TTF_CloseFont(font);

            if (auto it = mOpenFontFiles.find(font); it != mOpenFontFiles.end()) {
                mVFS.CloseFile(it->second);
                mOpenFontFiles.erase(it);
            }

            mFontSources.erase(font);
            closed.insert(font);
        };

        for (auto& [_, atlas] : mAtlases) {
            if (atlas.texture) mRenderer.UnloadTex(atlas.texture);
            if (atlas.atlasSurface) SDL_DestroySurface(atlas.atlasSurface);
            closeFont(atlas.font);
        }

        for (auto& [_, font] : mFallbackFonts)
            closeFont(font);

        mAtlases.clear();
        mFallbackFonts.clear();
        mFontSources.clear();
        mOpenFontFiles.clear();

        TTF_Quit();
    }

    std::string FontManager::MakeAtlasKey(const std::string& familyName, int pointSize) {
        return familyName + "@" + std::to_string(pointSize);
    }

    bool FontManager::Load(const std::string& path, const std::string& name, int size) {
        if (name.empty() || path.empty())
            return false;

        auto famIt = mFamilies.find(name);
        if (famIt != mFamilies.end() && !famIt->second.isFallback && famIt->second.path != path) {
            Unload(name);
        }

        mFamilies[name] = {path, false};

        if (!GetOrCreateAtlas(name, size))
            return false;

        if (mDefaultFontName.empty() || mDefaultFontName == kFallbackFamilyName)
            mDefaultFontName = name;

        return true;
    }

    void FontManager::Update() {
        for (auto& [_, atlas] : mAtlases)
            UpdateAtlasTexture(atlas);
    }

    TTF_Font* FontManager::LoadFontFromVFS(const std::string& path, int size) {
        VirtualFile* file = mVFS.OpenFile(path.c_str());
        if (!file || !file->sdl_stream)
            return nullptr;

        TTF_Font* font = TTF_OpenFontIO(file->sdl_stream, 0, size);
        if (font) {
            mFontSources[font] = {path, false};
            mOpenFontFiles[font] = file;
            return font;
        }

        mVFS.CloseFile(file);
        return nullptr;
    }

    TTF_Font* FontManager::GetFallbackFont(int size) {
        auto it = mFallbackFonts.find(size);
        if (it != mFallbackFonts.end())
            return it->second;

        SDL_IOStream* stream =
            SDL_IOFromMem(Default::default_ce_font, Default::default_ce_font_len);

        TTF_Font* font = TTF_OpenFontIO(stream, 1, size);
        if (!font) {
            SDL_CloseIO(stream);
            return nullptr;
        }

        mFallbackFonts[size] = font;
        mFontSources[font] = {"", true};

        return font;
    }

    FontManager::FontAtlas* FontManager::GetOrCreateAtlas(const std::string& familyName, int pointSize) {
        if (familyName.empty() || pointSize <= 0)
            return nullptr;

        const auto familyIt = mFamilies.find(familyName);
        if (familyIt == mFamilies.end())
            return nullptr;

        const std::string key = MakeAtlasKey(familyName, pointSize);
        auto atlasIt = mAtlases.find(key);
        if (atlasIt != mAtlases.end())
            return &atlasIt->second;

        const FontFamily& family = familyIt->second;
        TTF_Font* font = nullptr;

        if (family.isFallback) {
            font = GetFallbackFont(pointSize);
        } else {
            font = LoadFontFromVFS(family.path, pointSize);
        }

        if (!font && !family.isFallback) {
            font = GetFallbackFont(pointSize);
        }
        if (!font)
            return nullptr;

        BuildAtlas(key, font, pointSize);
        return &mAtlases.find(key)->second;
    }

    void FontManager::BuildAtlas(const std::string& name, TTF_Font* font, int size) {
        FontAtlas atlas;
        atlas.font = font;
        atlas.fontSize = size;

        atlas.atlasSurface =
            SDL_CreateSurface(ATLAS_W, ATLAS_H, SDL_PIXELFORMAT_RGBA32);

        SDL_FillSurfaceRect(atlas.atlasSurface, nullptr, 0x00000000);

        mAtlases[name] = std::move(atlas);
    }

    void FontManager::UpdateAtlasTexture(FontAtlas& atlas) {
        if (!atlas.atlasSurface || !atlas.dirty)
            return;

        auto* tex = mRenderer.CreateTextureFromData(
            atlas.atlasSurface->w,
            atlas.atlasSurface->h,
            atlas.atlasSurface->pixels,
            Renderer::TextureFormat::RGBA8,
            atlas.atlasSurface->pitch
        );

        if (atlas.texture)
            mRenderer.UnloadTex(atlas.texture);

        atlas.texture = tex;
        atlas.dirty = false;
    }

    bool FontManager::EnsureGlyph(FontAtlas& atlas, uint32_t cp) {
        if (atlas.glyphs.contains(cp))
            return true;

        SDL_Color white{255,255,255,255};

        SDL_Surface* glyph =
            TTF_RenderGlyph_Blended(atlas.font, (int)cp, white);
        if (!glyph) return false;

        int w = std::max(1, glyph->w);
        int h = std::max(1, glyph->h);

        int minx, maxx, miny, maxy, adv;
        TTF_GetGlyphMetrics(atlas.font, cp, &minx, &maxx, &miny, &maxy, &adv);

        if (atlas.penX + w >= ATLAS_W) {
            atlas.penX = 0;
            atlas.penY += atlas.rowH;
            atlas.rowH = 0;
        }

        SDL_Rect dst{atlas.penX, atlas.penY, w, h};
        SDL_BlitSurface(glyph, nullptr, atlas.atlasSurface, &dst);

        Glyph g{};
        g.u0 = atlas.penX / (float)ATLAS_W;
        g.v0 = atlas.penY / (float)ATLAS_H;
        g.u1 = (atlas.penX + w) / (float)ATLAS_W;
        g.v1 = (atlas.penY + h) / (float)ATLAS_H;

        g.w = w;
        g.h = h;
        g.advance = adv;
        g.bearingX = minx;
        g.bearingY = maxy;
        g.font = atlas.font;

        atlas.glyphs[cp] = g;

        atlas.penX += w;
        atlas.rowH = std::max(atlas.rowH, h);

        SDL_DestroySurface(glyph);
        atlas.dirty = true;

        return true;
    }

    void FontManager::DrawEx(const std::string& text,
                            const std::string& name,
                            int x,int y,float size,
                            Renderer::Colour col)
    {
        const int desiredSize = std::max(1, (int)std::lround(size));
        FontAtlas* atlasPtr = GetOrCreateAtlas(name, desiredSize);
        if (!atlasPtr) {
            atlasPtr = GetOrCreateAtlas(kFallbackFamilyName, desiredSize);
        }
        if (!atlasPtr)
            return;

        FontAtlas& atlas = *atlasPtr;

        float scale = size / atlas.fontSize;
        float cx = (float)x;

        uint32_t prev=0, cp=0;
        size_t i=0;

        while(i<text.size()) {
            DecodeUTF8(text,cp,i);
            if(!EnsureGlyph(atlas,cp)) continue;

            auto& g = atlas.glyphs[cp];

            if(prev) {
                int kern=0;
                if(TTF_GetGlyphKerning(g.font,prev,cp,&kern))
                    cx += kern * scale;
            }

            float drawX = std::round(cx + g.bearingX * scale);

            mRenderer.DrawTexUV(
                atlas.texture,
                drawX,
                (float)y,
                g.w*scale,
                g.h*scale,
                g.u0,g.v0,g.u1,g.v1,
                col,0.0f
            );

            cx += g.advance*scale;
            prev=cp;
        }
    }

    bool FontManager::DecodeUTF8(const std::string& text, uint32_t& out, size_t& cur) const {
        if (cur >= text.size()) return false;

        unsigned char c = text[cur];
        if (c < 0x80) { out=c; cur++; return true; }

        int extra = (c & 0xE0)==0xC0 ? 1 : (c & 0xF0)==0xE0 ? 2 : 3;
        uint32_t cp = c & (0x7F >> extra);

        for(int i=1;i<=extra;i++) {
            cp = (cp<<6) | (text[cur+i]&0x3F);
        }

        cur += extra+1;
        out = cp;
        return true;
    }

    void FontManager::Unload(const std::string& name) {
        if (name.empty())
            return;

        std::vector<std::string> toErase;
        const std::string prefix = name + "@";

        for (const auto& [key, _] : mAtlases) {
            if (key.rfind(prefix, 0) == 0)
                toErase.push_back(key);
        }

        std::unordered_set<TTF_Font*> closed;

        for (const auto& key : toErase) {
            auto it = mAtlases.find(key);
            if (it == mAtlases.end())
                continue;

            FontAtlas& atlas = it->second;

            if (atlas.texture) mRenderer.UnloadTex(atlas.texture);
            if (atlas.atlasSurface) SDL_DestroySurface(atlas.atlasSurface);

            if (atlas.font && !closed.count(atlas.font)) {
                TTF_CloseFont(atlas.font);
                mFontSources.erase(atlas.font);

                if (auto openIt = mOpenFontFiles.find(atlas.font); openIt != mOpenFontFiles.end()) {
                    mVFS.CloseFile(openIt->second);
                    mOpenFontFiles.erase(openIt);
                }

                closed.insert(atlas.font);
            }

            mAtlases.erase(it);
        }

        mFamilies.erase(name);

        if (mDefaultFontName == name)
            mDefaultFontName = kFallbackFamilyName;
    }

    void FontManager::UnloadAll() {
        std::unordered_set<TTF_Font*> closed;

        auto closeFont = [&](TTF_Font* font) {
            if (!font || closed.count(font))
                return;

            TTF_CloseFont(font);

            if (auto it = mOpenFontFiles.find(font); it != mOpenFontFiles.end()) {
                mVFS.CloseFile(it->second);
                mOpenFontFiles.erase(it);
            }

            mFontSources.erase(font);
            closed.insert(font);
        };

        for (auto& [_, atlas] : mAtlases) {
            if (atlas.texture) mRenderer.UnloadTex(atlas.texture);
            if (atlas.atlasSurface) SDL_DestroySurface(atlas.atlasSurface);
            closeFont(atlas.font);
        }

        for (auto& [_, font] : mFallbackFonts)
            closeFont(font);

        mAtlases.clear();
        mFallbackFonts.clear();
        mFontSources.clear();
        mOpenFontFiles.clear();
        mFamilies.clear();

        mFamilies[kFallbackFamilyName] = {"", true};
        mDefaultFontName = kFallbackFamilyName;
    }

    void FontManager::SetDefault(const std::string& name) {
        if (mFamilies.contains(name))
            mDefaultFontName = name;
    }

    void FontManager::Draw(const std::string& text, int x, int y, float size, Renderer::Colour colour) {
        DrawEx(text, mDefaultFontName, x, y, size, colour);
    }

}