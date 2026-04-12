#pragma once

#include <unordered_map>
#include <string>

#include "engine/renderer.hpp"
#include "engine/common/vfs.hpp"

namespace CE::Assets::Textures {
    class TextureManager {
        public:
            void Init(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs);
            void Load(const char* filepath, const char* name);
            void Unload(std::string& name);
            void Draw();
            void UnloadAll();
            void Shutdown();

        private:
            CE::Renderer::IRenderer* gRenderer;
            CE::VFS::VFS* gVFS;
            std::unordered_map<std::string, CE::Renderer::Texture*> gTextures;
    };
}