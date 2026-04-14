#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/common/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Textures {
    TextureManager::TextureManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
        gErrorTex = gRenderer->GetErrorTexture();
    }

    void TextureManager::Load(const char* filepath, const char* name) {
        CE::Renderer::Texture* tex;
        TMTexture texinfo = {};
        if (!gVFS->FileExists(filepath)) {
            CE::Log(LogLevel::Error, "[Texture Manager] Missing asset: {}", filepath);
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

    void TextureManager::Draw(const char* name, float x, float y, float w, float h, CE::Renderer::Colour colour) {
        auto tex = gTextures.find(name);
        if (tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                gRenderer->DrawTex(tex->second.Texture, x, y, w, h, colour);
                return;
            } else {
                if (!tex->second.ShownMissingError) {
                    CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw a missing asset: {}", tex->second.Path);
                    tex->second.ShownMissingError = true;
                }
                gRenderer->DrawTex(tex->second.Texture, x, y, w, h, {255, 255, 255, 255});
                return;
            }
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw an unloaded or missing asset: {}", name);
        gRenderer->DrawTex(gErrorTex, x, y, w, h, {255, 255, 255, 255});
    }

    void TextureManager::Unload(const char* name) {
        auto tex = gTextures.find(name);
        if(tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                gRenderer->UnloadTex(tex->second.Texture);
                CE::Log(LogLevel::Info, "[Texture Manager] Unloaded texture {}", name);
            } else {
                CE::Log(LogLevel::Warn, "[Texture Manager] Can not unload error texture! \n                 Deleting from texture list");
            }
            gTextures.erase(name);
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Can not unload a non-existant texture");
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

    TextureManager::~TextureManager() {
        UnloadAll();  
    }
}