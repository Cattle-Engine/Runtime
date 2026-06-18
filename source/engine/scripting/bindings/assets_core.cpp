#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

#include "engine/rendering/common/primitives_3d.hpp"

#include <cstdint>
#include <new>

namespace CE::Scripting {
    void ASMeshData::SetColour(const Renderer::Colour& colour) {
        Renderer::Primitives3D::SetMeshColour(mesh, colour);
    }

    static ASMeshData* MeshDataFactory() {
        return new ASMeshData();
    }

    void Runtime::ConstructColour(Renderer::Colour* self) {
        new (self) Renderer::Colour();
    }

    void Runtime::ConstructColourRGBA(
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a,
        Renderer::Colour* self
    ) {
        new (self) Renderer::Colour {r, g, b, a};
    }

    bool Runtime::RegisterAssetCoreBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics");

        CE_REGISTER_TYPE(
            "Colour",
            sizeof(Renderer::Colour),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Colour>()
        );

        if (mScriptEngine->RegisterObjectBehaviour(
                "Colour",
                asBEHAVE_CONSTRUCT,
                "void f()",
                asFUNCTION(ConstructColour),
                asCALL_CDECL_OBJLAST) < 0) {
            return false;
        }

        if (mScriptEngine->RegisterObjectBehaviour(
                "Colour",
                asBEHAVE_CONSTRUCT,
                "void f(uint8 r, uint8 g, uint8 b, uint8 a)",
                asFUNCTION(ConstructColourRGBA),
                asCALL_CDECL_OBJLAST) < 0) {
            return false;
        }

        if (mScriptEngine->RegisterObjectProperty("Colour", "uint8 r", asOFFSET(Renderer::Colour, r)) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterObjectProperty("Colour", "uint8 g", asOFFSET(Renderer::Colour, g)) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterObjectProperty("Colour", "uint8 b", asOFFSET(Renderer::Colour, b)) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterObjectProperty("Colour", "uint8 a", asOFFSET(Renderer::Colour, a)) < 0) {
            return false;
        }

        CE_REGISTER_TYPE("MeshData", 0, asOBJ_REF);

        if (mScriptEngine->RegisterObjectBehaviour(
                "MeshData",
                asBEHAVE_FACTORY,
                "MeshData@ f()",
                asFUNCTION(MeshDataFactory),
                asCALL_CDECL) < 0) {
            return false;
        }

        if (mScriptEngine->RegisterObjectBehaviour(
                "MeshData",
                asBEHAVE_ADDREF,
                "void f()",
                asMETHOD(ASMeshData, AddRef),
                asCALL_THISCALL) < 0) {
            return false;
        }

        if (mScriptEngine->RegisterObjectBehaviour(
                "MeshData",
                asBEHAVE_RELEASE,
                "void f()",
                asMETHOD(ASMeshData, Release),
                asCALL_THISCALL) < 0) {
            return false;
        }

        if (mScriptEngine->RegisterObjectMethod(
                "MeshData",
                "void SetColour(const Colour &in)",
                asMETHOD(ASMeshData, SetColour),
                asCALL_THISCALL) < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }
}
