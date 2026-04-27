#pragma once

#include <string>
#include <unordered_map>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>

#include "engine/renderer.hpp"
#include "engine/common/vfs.hpp"

namespace CE::Assets::Fonts {
    class FontManager {
        public:
            FontManager(Renderer::IRenderer& renderer, VFS::VFS& vfs, uint64_t instance_id);
            ~FontManager();

            void Update();

            void Draw(const std::string& text, int x, int y, float size, Renderer::Colour colour);
            void DrawEx(const std::string& text, const std::string& name, int x, int y, float size, Renderer::Colour colour);
            bool Load(const std::string& path, const std::string& name, int size = 24);
            void SetDefault(const std::string& name);
            void Unload(const std::string& name);
            void UnloadAll();
        private:
            struct Glyph {
                float u0, v0, u1, v1;
                int w, h;
                int advance;
                int bearingX, bearingY;
                TTF_Font* font;
            };

            struct KerningKey {
                uint32_t prev;
                uint32_t curr;
                bool operator==(const KerningKey& other) const {
                    return prev == other.prev && curr == other.curr;
                }
            };

            struct KerningKeyHash {
                std::size_t operator()(const KerningKey& key) const {
                    return std::hash<uint32_t>()(key.prev) ^ std::hash<uint32_t>()(key.curr);
                }
            };

        struct FontAtlas {
            Renderer::Texture* texture = nullptr;
            SDL_Surface* atlasSurface = nullptr;
            TTF_Font* font = nullptr;
            int fontSize = 0;   
            std::unordered_map<uint32_t, Glyph> glyphs;
            std::unordered_map<KerningKey, int, KerningKeyHash> kerningCache;
            int penX = 0;
            int penY = 0;
            int rowH = 0;
            bool dirty = false;
        };

            void BuildAtlas(const std::string& name, TTF_Font* font, int fontSize);
            bool EnsureGlyph(FontAtlas& atlas, uint32_t codepoint);
            void UpdateAtlasTexture(FontAtlas& atlas);
            bool DecodeUTF8(const std::string& text, uint32_t& outCodepoint, size_t& cursor) const;
            TTF_Font* LoadFontFromVFS(const std::string& path, int pointSize);
            TTF_Font* GetFallbackFont(int pointSize);

            std::string mDefaultFontName;
            std::unordered_map<int, TTF_Font*> mFallbackFonts;

            VFS::VFS& mVFS;
            Renderer::IRenderer& mRenderer;

            int mInstanceID;
            std::unordered_map<std::string, FontAtlas> mAtlases;
    };
}