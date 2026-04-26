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
        
            void Draw(std::string text, int x, int y, float size , Renderer::Colour colour);
            void DrawEx(std::string text, std::string name ,int x, int y, float size , Renderer::Colour colour);
            void Load(std::string path, std::string name);
            void SetDefault(std::string name);
            void Unload(std::string name);
            void UnloadAll();
        private:
            uint64_t mInstanceID;
            TTF_Font* mDefaultFont;
            TTF_Font* mFallBackFont;

            VFS::VFS& mVFS;
            Renderer::IRenderer& mRenderer;

            std::unordered_map<std::string, TTF_Font*> mFonts;
    };
}