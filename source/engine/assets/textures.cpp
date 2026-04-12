#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/common/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Textures {
    void TextureManager::Init(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
    }

    void TextureManager::Load(const char* filepath, const char* name) {
        if (!gVFS->FileExists(filepath)) {
            CE::Log(LogLevel::Error, "[Texture Manager] Missing asset: {}", filepath);
        }
        CE::Renderer::Texture* tex;

        tex = gRenderer->LoadTex(filepath);

        gTextures[name] = tex;
    }
}