#include "engine/rendering/common/primitives_3d.hpp"
#include "engine/scripting/angelscript.hpp"

namespace CE::Scripting {
    static ASMeshData* CreateCubeMesh(float width, float height, float depth) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCube({width, height, depth});
        return result;
    }

    static ASMeshData* CreateCubeMeshCol(float width, float height, float depth, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCube({width, height, depth}, colour);
        return result;
    }

    static ASMeshData* CreatePlaneMesh(float width, float depth) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreatePlane({width, depth});
        return result;
    }

    static ASMeshData* CreatePlaneMeshCol(float width, float depth, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreatePlane({width, depth}, colour);
        return result;
    }

    static ASMeshData* CreateSphereMesh(float radius, int segments, int rings) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateSphere(radius, segments, rings);
        return result;
    }

    static ASMeshData* CreateSphereMeshCol(float radius, int segments, int rings, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateSphere(radius, segments, rings, colour);
        return result;
    }

    static ASMeshData* CreateCylinderMesh(float radius, float height, int segments) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCylinder(radius, height, segments);
        return result;
    }

    static ASMeshData* CreateCylinderMeshCol(float radius, float height, int segments, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCylinder(radius, height, segments, colour);
        return result;
    }

    static ASMeshData* CreateConeMesh(float radius, float height, int segments) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCone(radius, height, segments);
        return result;
    }

    static ASMeshData* CreateConeMeshCol(float radius, float height, int segments, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCone(radius, height, segments, colour);
        return result;
    }

    static ASMeshData* CreateTorusMesh(float radius, float tubeRadius, int segments, int tubeSegments) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateTorus(radius, tubeRadius, segments, tubeSegments);
        return result;
    }

    static ASMeshData* CreateTorusMeshCol(float radius, float tubeRadius, int segments, int tubeSegments,
                                          const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateTorus(radius, tubeRadius, segments, tubeSegments, colour);
        return result;
    }

    static ASMeshData* CreateCapsuleMesh(float radius, float height, int segments) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCapsule(radius, height, segments);
        return result;
    }

    static ASMeshData* CreateCapsuleMeshCol(float radius, float height, int segments, const Renderer::Colour& colour) {
        ASMeshData* result = new ASMeshData();
        result->mesh = Renderer::Primitives3D::CreateCapsule(radius, height, segments, colour);
        return result;
    }

    bool Runtime::RegisterAssetMeshBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics");

        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreateCube(float width, float height, float depth)",
                                                  asFUNCTION(CreateCubeMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateCube(float width, float height, float depth, const Colour &in colour)",
                asFUNCTION(CreateCubeMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreatePlane(float width, float depth)",
                                                  asFUNCTION(CreatePlaneMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreatePlane(float width, float depth, const Colour &in colour)",
                asFUNCTION(CreatePlaneMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreateSphere(float radius, int segments, int rings)",
                                                  asFUNCTION(CreateSphereMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateSphere(float radius, int segments, int rings, const Colour &in colour)",
                asFUNCTION(CreateSphereMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreateCylinder(float radius, float height, int segments)",
                                                  asFUNCTION(CreateCylinderMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateCylinder(float radius, float height, int segments, const Colour &in colour)",
                asFUNCTION(CreateCylinderMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreateCone(float radius, float height, int segments)",
                                                  asFUNCTION(CreateConeMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateCone(float radius, float height, int segments, const Colour &in colour)",
                asFUNCTION(CreateConeMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateTorus(float radius, float tubeRadius, int segments, int tubeSegments)",
                asFUNCTION(CreateTorusMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateTorus(float radius, float tubeRadius, int segments, int "
                "tubeSegments, const Colour &in colour)",
                asFUNCTION(CreateTorusMeshCol), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction("MeshData@ CreateCapsule(float radius, float height, int segments)",
                                                  asFUNCTION(CreateCapsuleMesh), asCALL_CDECL) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterGlobalFunction(
                "MeshData@ CreateCapsule(float radius, float height, int segments, const Colour &in colour)",
                asFUNCTION(CreateCapsuleMeshCol), asCALL_CDECL) < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }
} // namespace CE::Scripting
