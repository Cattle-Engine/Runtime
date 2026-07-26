#include <angelscript.h>
#include <glm/ext/vector_float2_precision.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

#include "engine/scripting/bindings/rendering/rendering_types.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"

namespace {
    // just for good practice
    template<typename T>
    void Destruct(T *self) {
        self->~T();
    }

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

    static void Vec2Default(glm::vec2* self) {
        new(self) glm::vec2();
    }

    static void Vec2Init(float x, float y, glm::vec2* self) {
        new(self) glm::vec2(x, y);
    }

    static void Vec2Copy(const glm::vec2& other, glm::vec2* self) {
        new(self) glm::vec2(other);
    }

    static glm::vec2 Vec2Add(const glm::vec2& rhs, const glm::vec2* self) {
        return *self + rhs;
    }

    static glm::vec2 Vec2Subtract(const glm::vec2& rhs, const glm::vec2* self) {
        return *self - rhs;
    }

    static glm::vec2 Vec2Divide(const glm::vec2& rhs, const glm::vec2* self) {
        return *self / rhs;
    }

    static glm::vec2 Vec2Multiply(const glm::vec2& rhs, const glm::vec2* self) {
        return *self * rhs;
    }

    static glm::vec2& Vec2Assign(const glm::vec2& rhs, glm::vec2* self) {
        *self = rhs;
        return *self;
    }

    static glm::vec2 Vec2MultiplyScalar(float rhs, const glm::vec2* self) {
        return *self * rhs;
    }

    static glm::vec2 Vec2ScalarMultiply(float lhs, const glm::vec2* self) {
        return lhs * (*self);
    }

    static bool Vec2Equals(const glm::vec3& rhs, const glm::vec3* self) {
        return *self == rhs;
    }

    static void Vec3Default(glm::vec3* self) {
        new(self) glm::vec3();
    }

    static void Vec3Init(float x, float y, float z, glm::vec3* self) {
        new(self) glm::vec3(x, y, z);
    }

    static void Vec3Copy(const glm::vec3& other, glm::vec3* self) {
        new(self) glm::vec3(other);
    }

    static glm::vec3& Vec3Assign(const glm::vec3& rhs, glm::vec3* self) {
        *self = rhs;
        return *self;
    }

    static glm::vec3 Vec3Add(const glm::vec3& rhs, const glm::vec3* self) {
        return *self + rhs;
    }

    static glm::vec3 Vec3Subtract(const glm::vec3& rhs, const glm::vec3* self) {
        return *self - rhs;
    }

    static glm::vec3 Vec3Divide(const glm::vec3& rhs, const glm::vec3* self) {
        return *self / rhs;
    }

    static glm::vec3 Vec3Multiply(const glm::vec3& rhs, const glm::vec3* self) {
        return *self * rhs;
    }

    static float Vec3Length(const glm::vec3* self) {
        return glm::length(*self);
    }

    static glm::vec3 Vec3Normalised(const glm::vec3* self) {
        return glm::normalize(*self);
    }

    static glm::vec3 Vec3Cross(const glm::vec3& rhs, const glm::vec3* self) {
        return glm::cross(*self, rhs);
    }

    static float Vec3Dot(const glm::vec3& rhs, const glm::vec3* self) {
        return glm::dot(*self, rhs);
    }

    static glm::vec3 Vec3MultiplyScalar(float rhs, const glm::vec3* self) {
        return *self * rhs;
    }

    static glm::vec3 Vec3ScalarMultiply(float lhs, const glm::vec3* self) {
        return lhs * (*self);
    }

    static bool Vec3Equals(const glm::vec3& rhs, const glm::vec3* self) {
        return *self == rhs;
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

    bool RenderingTypes::RegisterVec3AndVec2() {
        mScriptEngine.SetDefaultNamespace("CE");
        CE_REGISTER_TYPE("Vec3", sizeof(glm::vec3), asOBJ_VALUE | asGetTypeTraits<glm::vec3>());
        CE_REGISTER_TYPE("Vec2", sizeof(glm::vec2), asOBJ_VALUE | asGetTypeTraits<glm::vec2>());

        // Register Vec2 behaviour
        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec2", 
            asBEHAVE_CONSTRUCT, 
            "void f()", 
            asFUNCTION(Vec2Default), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec2", 
            asBEHAVE_CONSTRUCT, 
            "void f(float, float)", 
            asFUNCTION(Vec2Init), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec2", 
            asBEHAVE_CONSTRUCT, 
            "void f(const Vec2& in)",
            asFUNCTION(Vec2Copy), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec2",
            "Vec2 opMul(float)",
            asFUNCTION(Vec2MultiplyScalar),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec2",
            "Vec2 opRMul(float)",
            asFUNCTION(Vec2ScalarMultiply),
            asCALL_CDECL_OBJLAST
        );

        // glm::vec2 is a trivial type but I like to be explicit
        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec2", 
            asBEHAVE_DESTRUCT, 
            "void f()", 
            asFUNCTION(Destruct<glm::vec2>),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec2",
            "bool opEquals(const Vec2& in) const",
            asFUNCTION(Vec2Equals),
            asCALL_CDECL_OBJLAST
        );

        // Expose x/y
        CE_REGISTER_OBJECT_PROPERTY(
            "Vec2", 
            "float x", 
            asOFFSET(glm::vec2, x)
        );

        CE_REGISTER_OBJECT_PROPERTY(
            "Vec2",
            "float y", 
            asOFFSET(glm::vec2, y)
        );

        // Expose arithmatic functions

        // addition 
        CE_REGISTER_OBJECT_METHOD(
            "Vec2", 
            "Vec2 opAdd(const Vec2& in)", 
            asFUNCTION(Vec2Add),
            asCALL_CDECL_OBJLAST
        );

        // subtraction
        CE_REGISTER_OBJECT_METHOD(
            "Vec2", 
            "Vec2 opSub(const Vec2& in)", 
            asFUNCTION(Vec2Subtract), 
            asCALL_CDECL_OBJLAST
        );

        // division
        CE_REGISTER_OBJECT_METHOD(
            "Vec2", 
            "Vec2 opDiv(const Vec2& in)", 
            asFUNCTION(Vec2Divide), 
            asCALL_CDECL_OBJLAST
        );

        // multiply
        CE_REGISTER_OBJECT_METHOD(
            "Vec2", 
            "Vec2 opMul(const Vec2& in)", 
            asFUNCTION(Vec2Multiply), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec2", 
            "Vec2& opAssign(const Vec2& in)", 
            asFUNCTION(Vec2Assign),
            asCALL_CDECL_OBJLAST
        );

        // Register glm::vec3 behaviour
        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec3", 
            asBEHAVE_CONSTRUCT, 
            "void f()", 
            asFUNCTION(Vec3Default), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec3", 
            asBEHAVE_CONSTRUCT, 
            "void f(float, float, float)", 
            asFUNCTION(Vec3Init), 
            asCALL_CDECL_OBJLAST
        );        

        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec3", 
            asBEHAVE_CONSTRUCT, 
            "void f(const Vec3& in)",
            asFUNCTION(Vec3Copy), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_BEHAVIOUR(
            "Vec3", 
            asBEHAVE_DESTRUCT, 
            "void f()", 
            asFUNCTION(Destruct<glm::vec3>),
            asCALL_CDECL_OBJLAST
        );

        // Expose x/y/z
        CE_REGISTER_OBJECT_PROPERTY(
            "Vec3", 
            "float x", 
            asOFFSET(glm::vec3, x)
        );

        CE_REGISTER_OBJECT_PROPERTY(
            "Vec3",
            "float y", 
            asOFFSET(glm::vec3, y)
        );

        CE_REGISTER_OBJECT_PROPERTY(
            "Vec3",
            "float z", 
            asOFFSET(glm::vec3, z)
        );

        // Expose functions arithmatic functions
        // assignment
        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3& opAssign(const Vec3& in)", 
            asFUNCTION(Vec3Assign),
            asCALL_CDECL_OBJLAST
        );

        // addition 
        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 opAdd(const Vec3& in)", 
            asFUNCTION(Vec3Add),
            asCALL_CDECL_OBJLAST
        );

        // subtraction
        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 opSub(const Vec3& in)", 
            asFUNCTION(Vec3Subtract), 
            asCALL_CDECL_OBJLAST
        );

        // division
        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 opDiv(const Vec3& in)", 
            asFUNCTION(Vec3Divide), 
            asCALL_CDECL_OBJLAST
        );

        // multiply
        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 opMul(const Vec3& in)", 
            asFUNCTION(Vec3Multiply), 
            asCALL_CDECL_OBJLAST
        );

        // expose some useful as fu glm functions

        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "float Length() const", 
            asFUNCTION(Vec3Length),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 Normalise() const", 
            asFUNCTION(Vec3Normalised),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "float Dot(const Vec3& in) const",
            asFUNCTION(Vec3Dot), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3", 
            "Vec3 Cross(const Vec3& in) const", 
            asFUNCTION(Vec3Cross), 
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3",
            "Vec3 opMul(float)",
            asFUNCTION(Vec3MultiplyScalar),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3",
            "Vec3 opRMul(float)",
            asFUNCTION(Vec3ScalarMultiply),
            asCALL_CDECL_OBJLAST
        );

        CE_REGISTER_OBJECT_METHOD(
            "Vec3",
            "bool opEquals(const Vec3& in) const",
            asFUNCTION(Vec3Equals),
            asCALL_CDECL_OBJLAST
        );

        return true;
    }

    bool RenderingTypes::RegisterBindings() {
        if(!RegisterColourBinding()) return false;
        if (!RegisterResourceHandleBindings()) return false;
        if (!RegisterVec3AndVec2()) return false;

        return true;
    }
}