#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"
#include "engine/scripting/bindings/rendering/rendering_resources.hpp"

namespace CE::Scripting::Bindings {
    // Texture resource bindings
    void TextureResourceBindings::LoadTexture(const std::string& path, Renderer::Resources::TextureHandle& out) {
        out = mRuntime.mTextureManager.Load(path);
    }

    void TextureResourceBindings::UnloadTexture(const Renderer::Resources::TextureHandle& handle) {
        mRuntime.mTextureManager.Unload(handle);
    }

    bool TextureResourceBindings::RegisterBindings() {
        mScriptEngine.SetDefaultNamespace("CE");

        // Register TextureHandle as a type for angelscript
        CE_REGISTER_TYPE("Texture", sizeof(Renderer::Resources::TextureHandle), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Resources::TextureHandle>());
        if (mScriptEngine.RegisterObjectProperty("Texture", "uint64 handle", asOFFSET(Renderer::Resources::TextureHandle, id)) < 0) {
            return false;
        }

        // Register the functions
        mScriptEngine.SetDefaultNamespace("CE::Graphics::Textures");
        CE_REGISTER_GLOBAL(TextureResourceBindings, this, "void Load(const string& in path, CE::Texture& out texture)", LoadTexture);
        CE_REGISTER_GLOBAL(TextureResourceBindings, this, "void Unload(const CE::Texture& in)", UnloadTexture);
        return true;
    }

    // Material resource bindings
    bool MaterialResourceBindings::RegisterBindings() {
        mScriptEngine.SetDefaultNamespace("CE");

        int result = 0;

        CE_REGISTER_TYPE("Material", sizeof(Renderer::Resources::MaterialHandle), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Resources::MaterialHandle>());

        result = mScriptEngine.RegisterObjectProperty("MaterialHandle", "uint64 handle", asOFFSET(Renderer::Resources::MaterialHandle, id));
        if (result < 0) {
            return false;
        }

        return true;
    }
}