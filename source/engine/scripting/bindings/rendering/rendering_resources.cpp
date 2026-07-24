#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"
#include "engine/scripting/bindings/rendering/rendering_resources.hpp"

namespace {
    static bool MeshHandleEquals(
        const CE::Renderer::Resources::MaterialHandle& self, 
        const CE::Renderer::Resources::MaterialHandle& other
    ) {
        return self.id == other.id;
    }
    
    static bool MeshHandleEqualsInt(const CE::Renderer::Resources::MaterialHandle& self, int64_t other) {
        return self.id == static_cast<uint64_t>(other);
    }
    
    static bool TextureHandleEquals(
        const CE::Renderer::Resources::TextureHandle& self, 
        const CE::Renderer::Resources::TextureHandle& other
    ) {
        return self.id == other.id;
    }
    
    static bool TextureHandleEqualsInt(const CE::Renderer::Resources::TextureHandle& self, int64_t other) {
        return self.id == static_cast<uint64_t>(other);
    }
}

namespace CE::Scripting::Bindings {
    // Texture resource bindings
    void TextureResourceBindings::LoadTexture(const std::string& path, Renderer::Resources::TextureHandle& out) {
        out = mRuntime.mTextureManager.Load(path);
    }

    void TextureResourceBindings::UnloadTexture(const Renderer::Resources::TextureHandle& handle) {
        mRuntime.mTextureManager.Unload(handle);
    }

    bool TextureResourceBindings::RegisterBindings() {
        int result = 0;
        
        mScriptEngine.SetDefaultNamespace("CE");

        // Register TextureHandle as a type for angelscript
        CE_REGISTER_TYPE("Texture", sizeof(Renderer::Resources::TextureHandle), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Resources::TextureHandle>());
        if (mScriptEngine.RegisterObjectProperty("Texture", "uint64 handle", asOFFSET(Renderer::Resources::TextureHandle, id)) < 0) {
            return false;
        }

        result = mScriptEngine.RegisterObjectMethod(
            "Texture", 
            "bool opEquals(const Texture &in) const",
            asFUNCTION(TextureHandleEquals), 
            asCALL_CDECL_OBJFIRST
        );
        
        if (result < 0) {
            return false;
        }
        
        result = mScriptEngine.RegisterObjectMethod(
            "Texture", 
            "bool opEquals(int64) const", 
            asFUNCTION(TextureHandleEqualsInt), 
            asCALL_CDECL_OBJFIRST
        );
        
        if (result < 0) {
            return false;
        }
        
        // Register the functions
        mScriptEngine.SetDefaultNamespace("CE::Graphics::Textures");
        CE_REGISTER_GLOBAL(TextureResourceBindings, this, "void Load(const string& in path, CE::Texture& out texture)", LoadTexture);
        CE_REGISTER_GLOBAL(TextureResourceBindings, this, "void Unload(const CE::Texture& in)", UnloadTexture);
        return true;
    }

    // Material resource bindings
    void MaterialResourceBindings::CreateMaterial(
        Renderer::Resources::MaterialHandle& handle,
        const Renderer::Resources::TextureHandle& tex
    ) {
        handle = mRuntime.mMaterialManager.CreateMaterial(tex);
    }
    
    void MaterialResourceBindings::DestroyMaterialHandle(const Renderer::Resources::MaterialHandle& handle) {
        mRuntime.mMaterialManager.DestroyMaterial(handle);
    }
    
    void MaterialResourceBindings::SetMaterialAlbedo(
        const Renderer::Resources::MaterialHandle& handle, 
        const Renderer::Resources::TextureHandle tex
    ) {
        mRuntime.mMaterialManager.SetMaterialAlbedo(handle, tex);
    }
    
    void MaterialResourceBindings::SetMaterialTint(
        const Renderer::Resources::MaterialHandle& handle, 
        const Renderer::Colour& colour
    ) {
        mRuntime.mMaterialManager.SetMaterialColour(handle, colour);
    }
    
    void MaterialResourceBindings::SetMaterialRoughness(
        const Renderer::Resources::MaterialHandle& handle, 
        float roughness
    ) {
        mRuntime.mMaterialManager.SetMaterialRoughness(handle, roughness);
    }
    
    void MaterialResourceBindings::SetMaterialMetallic(
        const Renderer::Resources::MaterialHandle& handle, 
        float metallic
    ) {
        mRuntime.mMaterialManager.SetMaterialMetallic(handle, metallic);
    }
    
    bool MaterialResourceBindings::MaterialExists(const Renderer::Resources::MaterialHandle& handle) const {
        return mRuntime.mMaterialManager.GetMaterial(handle) != nullptr;
    }
    
    bool MaterialResourceBindings::RegisterBindings() {
        mScriptEngine.SetDefaultNamespace("CE");

        int result = 0;
        
        // Register CE::Renderer::Resources::MaterialHandle into CE::Material
        CE_REGISTER_TYPE("Material", sizeof(Renderer::Resources::MaterialHandle), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Resources::MaterialHandle>());

        result = mScriptEngine.RegisterObjectProperty("Material", "uint64 handle", asOFFSET(Renderer::Resources::MaterialHandle, id));
        if (result < 0) {
            return false;
        }
        
        result = mScriptEngine.RegisterObjectMethod(
            "Material", 
            "bool opEquals(const Material &in) const",
            asFUNCTION(MeshHandleEquals), 
            asCALL_CDECL_OBJFIRST
        );
        
        if (result < 0) {
            return false;
        }
        
        result = mScriptEngine.RegisterObjectMethod(
            "Material", 
            "bool opEquals(int64) const", 
            asFUNCTION(MeshHandleEqualsInt), 
            asCALL_CDECL_OBJFIRST
        );
        
        if (result < 0) {
            return false;
        }
        
        // Bind the functions
        mScriptEngine.SetDefaultNamespace("CE::Graphics::Materials");

        CE_REGISTER_GLOBAL(
            MaterialResourceBindings, 
            this, 
            "void CreateMaterial(Material& out handle, const Texture& in tex)", 
            CreateMaterial
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this,
            "void DestroyMaterial(Material& in handle)",
            DestroyMaterialHandle
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this,
            "void SetAlbedo(Material& in handle, const Texture& in tex)",
            SetMaterialAlbedo
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this, 
            "void SetTint(Material& in handle, const Colour& in colour)", 
            SetMaterialTint
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this,
            "void SetRoughness(Material& in handle, const float in roughness)",
            SetMaterialRoughness
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this, 
            "void SetMetallic(Material& in handle, const float in metallic)", 
            SetMaterialMetallic
        );
        
        CE_REGISTER_GLOBAL(
            MaterialResourceBindings,
            this,
            "bool MaterialExists(Material& in handle)",
            MaterialExists
        );
        
        return true;
    }
    
    
}