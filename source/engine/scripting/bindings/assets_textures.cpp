#include <cstdint>

#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

namespace {
    constexpr CE::Renderer::Colour kWhite{255, 255, 255, 255};
}

namespace CE::Scripting {
    void Runtime::LoadTexture(const std::string &path, TextureHandle &texture) {
        texture.handle = mTextureManager.Load(path);
    }

    void Runtime::UnloadTexture(const TextureHandle &texture) {
        mTextureManager.Unload(texture.handle);
    }

    void Runtime::DrawTexture(const TextureHandle &texture, int x, int y, bool flipX, bool flipY, float tileX,
                              float tileY) {
        CE::Renderer::Texture *tex = mTextureManager.GetTexture(texture.handle);
        if (!tex)
            return;

        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX && flipY) {
            flip = static_cast<CE::Renderer::TextureFlip>(static_cast<uint8_t>(CE::Renderer::TextureFlip::Horizontal) |
                                                          static_cast<uint8_t>(CE::Renderer::TextureFlip::Vertical));
        } else if (flipX) {
            flip = CE::Renderer::TextureFlip::Horizontal;
        } else if (flipY) {
            flip = CE::Renderer::TextureFlip::Vertical;
        }

        float width = static_cast<float>(tex->width) * tileX;
        float height = static_cast<float>(tex->height) * tileY;

        mRenderer.DrawTex(tex, static_cast<float>(x), static_cast<float>(y), width, height, kWhite, 0.0f, flip);
    }

    void Runtime::DrawTextureEx(const TextureHandle &texture, int x, int y, const Renderer::Colour &colour, bool flipX,
                                bool flipY, float tileX, float tileY) {
        CE::Renderer::Texture *tex = mTextureManager.GetTexture(texture.handle);
        if (!tex)
            return;

        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX && flipY) {
            flip = static_cast<CE::Renderer::TextureFlip>(static_cast<uint8_t>(CE::Renderer::TextureFlip::Horizontal) |
                                                          static_cast<uint8_t>(CE::Renderer::TextureFlip::Vertical));
        } else if (flipX) {
            flip = CE::Renderer::TextureFlip::Horizontal;
        } else if (flipY) {
            flip = CE::Renderer::TextureFlip::Vertical;
        }

        float width = static_cast<float>(tex->width) * tileX;
        float height = static_cast<float>(tex->height) * tileY;

        mRenderer.DrawTex(tex, static_cast<float>(x), static_cast<float>(y), width, height, colour, 0.0f, flip);
    }

    void Runtime::DrawTextureRot(const TextureHandle &texture, int x, int y, float rotation, bool flipX, bool flipY,
                                 float tileX, float tileY) {
        CE::Renderer::Texture *tex = mTextureManager.GetTexture(texture.handle);
        if (!tex)
            return;

        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX && flipY) {
            flip = static_cast<CE::Renderer::TextureFlip>(static_cast<uint8_t>(CE::Renderer::TextureFlip::Horizontal) |
                                                          static_cast<uint8_t>(CE::Renderer::TextureFlip::Vertical));
        } else if (flipX) {
            flip = CE::Renderer::TextureFlip::Horizontal;
        } else if (flipY) {
            flip = CE::Renderer::TextureFlip::Vertical;
        }

        float width = static_cast<float>(tex->width) * tileX;
        float height = static_cast<float>(tex->height) * tileY;

        mRenderer.DrawTex(tex, static_cast<float>(x), static_cast<float>(y), width, height, kWhite, rotation, flip);
    }

    void Runtime::DrawTextureRotEx(const TextureHandle &texture, int x, int y, float rotation,
                                   const Renderer::Colour &colour, bool flipX, bool flipY, float tileX, float tileY) {
        CE::Renderer::Texture *tex = mTextureManager.GetTexture(texture.handle);
        if (!tex)
            return;

        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX && flipY) {
            flip = static_cast<CE::Renderer::TextureFlip>(static_cast<uint8_t>(CE::Renderer::TextureFlip::Horizontal) |
                                                          static_cast<uint8_t>(CE::Renderer::TextureFlip::Vertical));
        } else if (flipX) {
            flip = CE::Renderer::TextureFlip::Horizontal;
        } else if (flipY) {
            flip = CE::Renderer::TextureFlip::Vertical;
        }

        float width = static_cast<float>(tex->width) * tileX;
        float height = static_cast<float>(tex->height) * tileY;

        mRenderer.DrawTex(tex, static_cast<float>(x), static_cast<float>(y), width, height, colour, rotation, flip);
    }

    void Runtime::DrawTexturePro(const TextureHandle &texture, int x, int y, int w, int h, float rotation,
                                 const Renderer::Colour &colour, bool flipX, bool flipY, float tileX, float tileY) {
        CE::Renderer::Texture *tex = mTextureManager.GetTexture(texture.handle);
        if (!tex)
            return;

        CE::Renderer::TextureFlip flip = CE::Renderer::TextureFlip::None;
        if (flipX && flipY) {
            flip = static_cast<CE::Renderer::TextureFlip>(static_cast<uint8_t>(CE::Renderer::TextureFlip::Horizontal) |
                                                          static_cast<uint8_t>(CE::Renderer::TextureFlip::Vertical));
        } else if (flipX) {
            flip = CE::Renderer::TextureFlip::Horizontal;
        } else if (flipY) {
            flip = CE::Renderer::TextureFlip::Vertical;
        }

        float width = static_cast<float>(w) * tileX;
        float height = static_cast<float>(h) * tileY;

        mRenderer.DrawTex(tex, static_cast<float>(x), static_cast<float>(y), width, height, colour, rotation, flip);
    }

    bool Runtime::RegisterAssetTextureBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE");

        CE_REGISTER_TYPE("Texture", sizeof(TextureHandle), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<TextureHandle>());

        if (mScriptEngine->RegisterObjectProperty("Texture", "uint64 handle", asOFFSET(TextureHandle, handle)) < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Textures");
        CE_REGISTER_GLOBAL("void LoadTexture(const string &in path, CE::Texture &out texture)", LoadTexture);
        CE_REGISTER_GLOBAL("void UnloadTexture(const CE::Texture &in texture)", UnloadTexture);
        CE_REGISTER_GLOBAL(
            "void DrawTexture(const CE::Texture &in texture, int x, int y, bool flipX = false, bool flipY = "
            "false, float tileX = 1.0f, float tileY = 1.0f)",
            DrawTexture);
        CE_REGISTER_GLOBAL(
            "void DrawTextureEx(const CE::Texture &in texture, int x, int y, const CE::Graphics::Colour &in "
            "colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            DrawTextureEx);
        CE_REGISTER_GLOBAL(
            "void DrawTextureRot(const CE::Texture &in texture, int x, int y, float rotation, bool flipX = "
            "false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            DrawTextureRot);
        CE_REGISTER_GLOBAL(
            "void DrawTextureRotEx(const CE::Texture &in texture, int x, int y, float rotation, const "
            "CE::Graphics::Colour "
            "&in colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            DrawTextureRotEx);
        CE_REGISTER_GLOBAL(
            "void DrawTexturePro(const CE::Texture &in texture, int x, int y, int w, int h, float rotation, "
            "const CE::Graphics::Colour &in colour, bool flipX = false, bool flipY = false, float tileX = "
            "1.0f, float tileY = 1.0f)",
            DrawTexturePro);
        mScriptEngine->SetDefaultNamespace("");
        return true;
    }
} // namespace CE::Scripting
