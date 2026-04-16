#pragma once

#include <unordered_map>
#include <string>

#include "engine/renderer.hpp"
#include "engine/common/vfs.hpp"

namespace CE::Assets::Textures {
    class TextureManager {
        public:
            TextureManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs);
            void Load(const char* filepath, const char* name);
            void Unload(const char* name);
            
            void DrawPro(const char* name, int x, int y, int w, 
                int h, float rotation, CE::Renderer::Colour colour);
            void Draw(const char* name, int x, int y, CE::Renderer::Colour colour);
            void DrawRot(const char* name, int x, int y, 
                float rotation, CE::Renderer::Colour colour);

            void UnloadAll();
            ~TextureManager();

        private:
            CE::Renderer::IRenderer* gRenderer;
            CE::VFS::VFS* gVFS;
            CE::Renderer::Texture* gErrorTex;
            // An internal struct to hold some metadata for a texture
            struct TMTexture {
                bool IsErrorTex = false;
                bool ShownMissingError = false;
                const char* Path = "Missing path";
                CE::Renderer::Texture* Texture;
            };
            std::unordered_map<std::string, TMTexture> gTextures;
    };
}