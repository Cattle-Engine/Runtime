#pragma once

#include <string>
#include <unordered_map>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>
#include <vector>

#include "engine/renderer.hpp"
#include "engine/common/fs/vfs.hpp"

namespace CE::Assets::Fonts {
    struct AtlasDebugInfo {
        std::string key;          // "Roboto@16"
        std::string familyName;
        int fontSize;

        int atlasWidth;
        int atlasHeight;

        int penX;
        int penY;
        int rowH;

        size_t glyphCount;

        bool hasTexture;
        bool dirty;

        size_t estimatedMemoryBytes;
    };
    
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

            Renderer::Texture* Debug_GetAtlasTex(const std::string& family, int size) const;
            std::vector<AtlasDebugInfo> Debug_GetAtlases() const;
            std::string Debug_GetDefaultFontName() const;
        private:
            struct FontFamily {
                std::string path;
                bool isFallback = false;
            };

            struct Glyph {
                float u0, v0, u1, v1;
                int w, h;
                int advance;
                int bearingX, bearingY;
                TTF_Font* font;
            };

        struct FontAtlas {
            Renderer::Texture* texture = nullptr;
            SDL_Surface* atlasSurface = nullptr;
            TTF_Font* font = nullptr;
            int fontSize = 0;   
            std::unordered_map<uint32_t, Glyph> glyphs;
            int penX = 0;
            int penY = 0;
            int rowH = 0;
            bool dirty = false;
        };

        struct FontSource {
            std::string path;
            bool isFallback;
        };
            void BuildAtlas(const std::string& name, TTF_Font* font, int fontSize);
            bool EnsureGlyph(FontAtlas& atlas, uint32_t codepoint);
            void UpdateAtlasTexture(FontAtlas& atlas);
            bool DecodeUTF8(const std::string& text, uint32_t& outCodepoint, size_t& cursor) const;
            TTF_Font* LoadFontFromVFS(const std::string& path, int pointSize);
            TTF_Font* GetFallbackFont(int pointSize);
            FontAtlas* GetOrCreateAtlas(const std::string& familyName, int pointSize);
            static std::string MakeAtlasKey(const std::string& familyName, int pointSize);

            std::string mDefaultFontName;
            std::unordered_map<int, TTF_Font*> mFallbackFonts;

            VFS::VFS& mVFS;
            Renderer::IRenderer& mRenderer;

            std::unordered_map<std::string, FontFamily> mFamilies;
            std::unordered_map<TTF_Font*, FontSource> mFontSources;
            std::unordered_map<TTF_Font*, VirtualFile*> mOpenFontFiles;

            int mInstanceID;
            std::unordered_map<std::string, FontAtlas> mAtlases;
    };
}
