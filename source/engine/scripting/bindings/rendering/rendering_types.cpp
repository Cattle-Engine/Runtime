#include "engine/scripting/bindings/rendering/rendering_types.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"

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
    
    void ConstructColour(CE::Renderer::Colour* self) {
        new (self) CE::Renderer::Colour();
    }
    
    void ConstructColourRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a, CE::Renderer::Colour* self) {
        new (self) CE::Renderer::Colour{r, g, b, a};
    }
}

namespace CE::Scripting::Bindings {
    bool RenderingTypes::RegisterColourBinding() {
        CE_REGISTER_TYPE("Colour", sizeof(Renderer::Colour), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Colour>());

        if (mScriptEngine.RegisterObjectBehaviour("Colour", asBEHAVE_CONSTRUCT, "void f()",
            asFUNCTION(ConstructColour), asCALL_CDECL_OBJLAST) < 0) {
            return false;
        }

        if (mScriptEngine.RegisterObjectBehaviour("Colour", asBEHAVE_CONSTRUCT,
            "void f(uint8 r, uint8 g, uint8 b, uint8 a)",
            asFUNCTION(ConstructColourRGBA), asCALL_CDECL_OBJLAST) < 0) {
            return false;
        }

        if (mScriptEngine.RegisterObjectProperty("Colour", "uint8 r", asOFFSET(Renderer::Colour, r)) < 0) {
            return false;
        }
        if (mScriptEngine.RegisterObjectProperty("Colour", "uint8 g", asOFFSET(Renderer::Colour, g)) < 0) {
            return false;
        }
        if (mScriptEngine.RegisterObjectProperty("Colour", "uint8 b", asOFFSET(Renderer::Colour, b)) < 0) {
            return false;
        }
        if (mScriptEngine.RegisterObjectProperty("Colour", "uint8 a", asOFFSET(Renderer::Colour, a)) < 0) {
            return false;
        }

        return true;
    }
    
    bool RenderingTypes::RegisterResourceHandleBindings() {
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
        
        
        // Register CE::Renderer::Resources::TextureHandle into CE::Texture
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
        
        return true;
    }

    bool RenderingTypes::RegisterBindings() {
        bool result;

        result = RegisterColourBinding();

        return result;
    }
}