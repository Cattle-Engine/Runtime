#include "engine/scripting/bindings/rendering/rendering_misc.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Scripting::Bindings {
    void ConstructColour(Renderer::Colour* self) {
        new (self) Renderer::Colour();
    }

    void ConstructColourRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a, Renderer::Colour* self) {
        new (self) Renderer::Colour{r, g, b, a};
    }

    bool RenderingMisc::RegisterColourBinding() {
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

    bool RenderingMisc::RegisterBindings() {
        bool result;

        result = RegisterColourBinding();

        return result;
    }
}