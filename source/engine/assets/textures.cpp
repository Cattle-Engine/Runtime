#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Textures {
    constexpr int ERROR_SIZE = 50;
    TextureManager::TextureManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
        gErrorTex = gRenderer->GetErrorTexture();
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
        int h, float rotation, CE::Renderer::Colour colour) {
        auto tex = gTextures.find(name);
        if (tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                gRenderer->DrawTex(tex->second.Texture,
                    x - w / 2, y - h / 2,   // offset so x,y is the centre
                    w, h, colour, rotation);
                return;
            } else {
                if (!tex->second.ShownMissingError) {
                    CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw a missing asset: {}", tex->second.Path);
                    tex->second.ShownMissingError = true;
                }
                gRenderer->DrawTex(tex->second.Texture,
                    x - w / 2, y - h / 2,
                    w, h, {255, 255, 255, 255}, 0.0f);
                return;
            }
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw an unloaded or missing asset: {}", name);
        gRenderer->DrawTex(gErrorTex, x - 8, y - 8, w, h, {255, 255, 255, 255}, 0.0f);
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

    void TextureManager::DrawRot(const char* name, int x, int y, 
            float rotation, CE::Renderer::Colour colour) {
        auto tex = gTextures.find(name);
        if (tex != gTextures.end()) {
            if (!tex->second.IsErrorTex) {
                int w = tex->second.Texture->width;
                int h = tex->second.Texture->height;
                gRenderer->DrawTex(tex->second.Texture,
                    x - w / 2, y - h / 2,   // offset so x,y is the centre
                    w, h, colour, rotation);
                return;
            } else {
                if (!tex->second.ShownMissingError) {
                    CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw a missing asset: {}", tex->second.Path);
                    tex->second.ShownMissingError = true;
                }
                gRenderer->DrawTex(tex->second.Texture,
                    x - 45 / 2,
                    y - 45 / 2,
                    ERROR_SIZE, ERROR_SIZE,
                    {255, 255, 255, 255}, rotation);
                return; 
            }
        }
        CE::Log(LogLevel::Error, "[Texture Manager] Tried to draw an unloaded or missing asset: {}", name);
        gRenderer->DrawTex(gErrorTex, x - 8, y - 8, ERROR_SIZE, ERROR_SIZE, {255, 255, 255, 255}, rotation);
    }

    void TextureManager::Draw(const char* name, int x, int y, CE::Renderer::Colour colour) {
        TextureManager::DrawRot(name, x, y, 0.0f, colour);
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