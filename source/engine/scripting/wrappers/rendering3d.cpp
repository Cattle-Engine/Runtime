#include "engine/scripting/angelscript.hpp"

#include "engine/common/tracelog.hpp"
#include "engine/rendering/common/gpu_mesh_manager.hpp"
#include "engine/rendering/common/material_manager.hpp"

#include <new>

namespace {
    constexpr const char* k3DNamespace = "CE::Graphics::Rendering3D";
}

namespace CE::Scripting {
    namespace {
        glm::vec3 ToVec3(const Vec3Desc& value) {
            return {value.x, value.y, value.z};
        }

        Renderer::Transform3D ToTransform(const Transform3DDesc& value) {
            return {
                ToVec3(value.position),
                ToVec3(value.rotation),
                ToVec3(value.scale)
            };
        }

        Renderer::Camera3D ToCamera(const Camera3DDesc& value) {
            Renderer::Camera3D camera {};
            camera.position = ToVec3(value.position);
            camera.rotation = ToVec3(value.rotation);
            camera.target = ToVec3(value.target);
            camera.up = ToVec3(value.up);
            camera.fov = value.fov;
            camera.nearClip = value.nearClip;
            camera.farClip = value.farClip;
            camera.orthoSize = value.orthoSize;
            camera.aspectOverride = value.aspectOverride;
            camera.useTarget = value.useTarget;
            camera.projection = value.projection;
            return camera;
        }

        Vec3Desc FromVec3(const glm::vec3& value) {
            return {value.x, value.y, value.z};
        }

        Camera3DDesc FromCamera(const Renderer::Camera3D& value) {
            return {
                FromVec3(value.position),
                FromVec3(value.rotation),
                FromVec3(value.target),
                FromVec3(value.up),
                value.fov,
                value.nearClip,
                value.farClip,
                value.orthoSize,
                value.aspectOverride,
                value.useTarget,
                value.projection
            };
        }
    }

    void Runtime::ConstructVec3(Vec3Desc* self) {
        new (self) Vec3Desc();
    }

    void Runtime::ConstructVec3XYZ(float x, float y, float z, Vec3Desc* self) {
        new (self) Vec3Desc {x, y, z};
    }

    void Runtime::ConstructTransform3D(Transform3DDesc* self) {
        new (self) Transform3DDesc();
    }

    void Runtime::ConstructCamera3D(Camera3DDesc* self) {
        new (self) Camera3DDesc();
    }

    void Runtime::ConstructMaterial(MaterialDesc* self) {
        new (self) MaterialDesc();
    }

    bool Runtime::Register3DBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        int result = 0;

        mScriptEngine->SetDefaultNamespace("CE");

        mScriptEngine->RegisterObjectType(
            "MaterialHandle",
            sizeof(CE::Renderer::Resources::MaterialHandle),
            asOBJ_VALUE | asOBJ_POD
        );

        mScriptEngine->RegisterObjectType(
            "MeshHandle",
            sizeof(CE::Renderer::Resources::MeshHandle),
            asOBJ_VALUE | asOBJ_POD
        );

        mScriptEngine->SetDefaultNamespace(k3DNamespace);

        result = mScriptEngine->RegisterEnum("CameraProjection");
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnumValue("CameraProjection", "Perspective", static_cast<int>(Renderer::Camera3D::ProjectionMode::Perspective));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnumValue("CameraProjection", "Orthographic", static_cast<int>(Renderer::Camera3D::ProjectionMode::Orthographic));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectType(
            "Vec3",
            sizeof(Vec3Desc),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vec3Desc>()
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Vec3",
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(ConstructVec3),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Vec3",
            asBEHAVE_CONSTRUCT,
            "void f(float x, float y, float z)",
            asFUNCTION(ConstructVec3XYZ),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Vec3", "float x", asOFFSET(Vec3Desc, x));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Vec3", "float y", asOFFSET(Vec3Desc, y));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Vec3", "float z", asOFFSET(Vec3Desc, z));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectType(
            "Transform3D",
            sizeof(Transform3DDesc),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Transform3DDesc>()
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Transform3D",
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(ConstructTransform3D),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Transform3D", "Vec3 position", asOFFSET(Transform3DDesc, position));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Transform3D", "Vec3 rotation", asOFFSET(Transform3DDesc, rotation));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Transform3D", "Vec3 scale", asOFFSET(Transform3DDesc, scale));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectType(
            "Camera3D",
            sizeof(Camera3DDesc),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Camera3DDesc>()
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Camera3D",
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(ConstructCamera3D),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "Vec3 position", asOFFSET(Camera3DDesc, position));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "Vec3 rotation", asOFFSET(Camera3DDesc, rotation));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "Vec3 target", asOFFSET(Camera3DDesc, target));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "Vec3 up", asOFFSET(Camera3DDesc, up));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "float fov", asOFFSET(Camera3DDesc, fov));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "float nearClip", asOFFSET(Camera3DDesc, nearClip));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "float farClip", asOFFSET(Camera3DDesc, farClip));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "float orthoSize", asOFFSET(Camera3DDesc, orthoSize));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "float aspectOverride", asOFFSET(Camera3DDesc, aspectOverride));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "bool useTarget", asOFFSET(Camera3DDesc, useTarget));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Camera3D", "CameraProjection projection", asOFFSET(Camera3DDesc, projection));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectType(
            "Material",
            sizeof(MaterialDesc),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<MaterialDesc>()
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Material",
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(ConstructMaterial),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Material", "CE::Graphics::Colour tint", asOFFSET(MaterialDesc, tint));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Material", "float roughness", asOFFSET(MaterialDesc, roughness));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Material", "float metallic", asOFFSET(MaterialDesc, metallic));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetCamera3D(const Camera3D &in camera)",
            asMETHOD(Runtime, SetCamera3D),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "Camera3D GetCamera3D()",
            asMETHOD(Runtime, GetCamera3D),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSunEnabled(bool enabled)",
            asMETHOD(Runtime, SetSunEnabled),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSunDirection(const Vec3 &in direction)",
            asMETHOD(Runtime, SetSunDirection),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSunPosition(const Vec3 &in position)",
            asMETHOD(Runtime, SetSunPosition),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSunTint(const Vec3 &in colour)",
            asMETHOD(Runtime, SetSunTint),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSunIntensity(float intensity)",
            asMETHOD(Runtime, SetSunIntensity),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetAmbientLight(const Vec3 &in colour, float intensity)",
            asMETHOD(Runtime, SetAmbientLight),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void LoadSkyBox(const string &in name, const string &in frontPath, const string &in backPath, const string &in leftPath, const string &in rightPath, const string &in topPath, const string &in bottomPath)",
            asMETHOD(Runtime, LoadSkyBox),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetSkyBox(const string &in name)",
            asMETHOD(Runtime, SetSkyBox),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void ClearSkyBox()",
            asMETHOD(Runtime, ClearSkyBox),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadSkyBox(const string &in name)",
            asMETHOD(Runtime, UnloadSkyBox),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "uint64 CreateMaterialHandle(const Material &in material, const string &in textureName = \"\")",
            asMETHODPR(Runtime, CreateMaterialHandle, (const MaterialDesc&, const std::string&), uint64_t),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DestroyMaterialHandle(uint64 handle)",
            asMETHODPR(Runtime, DestroyMaterialHandle, (uint64_t), void),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialAlbedo(uint64 handle, const string &in textureName)",
            asMETHODPR(Runtime, SetMaterialAlbedo, (uint64_t, const std::string&), bool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialTint(uint64 handle, const CE::Graphics::Colour &in colour)",
            asMETHODPR(Runtime, SetMaterialTint, (uint64_t, const Renderer::Colour&), bool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialRoughness(uint64 handle, float roughness)",
            asMETHODPR(Runtime, SetMaterialRoughness, (uint64_t, float), bool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialMetallic(uint64 handle, float metallic)",
            asMETHODPR(Runtime, SetMaterialMetallic, (uint64_t, float), bool),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool HasMaterial(uint64 handle)",
            asMETHOD(Runtime, HasMaterial),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "int DebugLoadedMaterialsCount()",
            asMETHOD(Runtime, DebugLoadedMaterialsCount),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "uint64 CreateMeshHandle(CE::Graphics::MeshData@ meshData)",
            asMETHOD(Runtime, CreateMeshHandle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void ChangeMesh(uint64 handle, CE::Graphics::MeshData@ meshData)",
            asMETHOD(Runtime, ChangeMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "uint64 DestroyMesh(uint64 handle)",
            asMETHOD(Runtime, DestroyMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool HasMesh(uint64 handle)",
            asMETHOD(Runtime, HasMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawMesh(uint64 handle, const Transform3D &in transform, uint64 materialHandle = 0, bool errorTexture = false)",
            asMETHOD(Runtime, DrawMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    void Runtime::SetCamera3D(const Camera3DDesc& camera) {
        mRenderer.SetCamera3D(ToCamera(camera));
    }

    Camera3DDesc Runtime::GetCamera3D() const {
        return FromCamera(mRenderer.GetCamera3DState());
    }

    void Runtime::SetSunEnabled(bool enabled) {
        mRenderer.SetSunEnabled(enabled);
    }

    void Runtime::SetSunDirection(const Vec3Desc& direction) {
        mRenderer.SetSunDirection(ToVec3(direction));
    }

    void Runtime::SetSunPosition(const Vec3Desc& position) {
        mRenderer.SetSunPosition(ToVec3(position));
    }

    void Runtime::SetSunTint(const Vec3Desc& colour) {
        mRenderer.SetSunTint(ToVec3(colour));
    }

    void Runtime::SetSunIntensity(float intensity) {
        mRenderer.SetSunIntensity(intensity);
    }

    void Runtime::SetAmbientLight(const Vec3Desc& colour, float intensity) {
        mRenderer.SetAmbientLight(ToVec3(colour), intensity);
    }

    void Runtime::LoadSkyBox(
        const std::string& name,
        const std::string& frontPath,
        const std::string& backPath,
        const std::string& leftPath,
        const std::string& rightPath,
        const std::string& topPath,
        const std::string& bottomPath
    ) {
        mSkyboxManager.Load(
            frontPath,
            backPath,
            leftPath,
            rightPath,
            topPath,
            bottomPath,
            name
        );
    }

    void Runtime::SetSkyBox(const std::string& name) {
        mSkyboxManager.Set(name.c_str());
    }

    void Runtime::ClearSkyBox() {
        mSkyboxManager.Set("");
    }

    void Runtime::UnloadSkyBox(const std::string& name) {
        mSkyboxManager.Unload(name.c_str());
    }
    uint64_t Runtime::CreateMaterialHandle(const MaterialDesc& material, const std::string& textureName) {
        Renderer::Resources::TextureHandle texHandle = 0;
        if (!textureName.empty()) {
            std::string tmp = textureName;
            texHandle = LoadTexture(tmp);
        }

        auto handle = mMaterialManager.CreateMaterial(texHandle);
        mMaterialManager.SetMaterialColour(handle, material.tint);
        mMaterialManager.SetMaterialRoughness(handle, material.roughness);
        mMaterialManager.SetMaterialMetallic(handle, material.metallic);
        return handle;
    }

    void Runtime::DestroyMaterialHandle(uint64_t handle) {
        mMaterialManager.DestroyMaterial(handle);
    }

    bool Runtime::SetMaterialAlbedo(uint64_t handle, const std::string& textureName) {
        if (textureName.empty()) return false;
        std::string tmp = textureName;
        auto tex = LoadTexture(tmp);
        mMaterialManager.SetMaterialAlbedo(handle, tex);
        return mMaterialManager.GetMaterial(handle) != nullptr;
    }

    bool Runtime::SetMaterialTint(uint64_t handle, const Renderer::Colour& colour) {
        mMaterialManager.SetMaterialColour(handle, colour);
        return mMaterialManager.GetMaterial(handle) != nullptr;
    }

    bool Runtime::SetMaterialRoughness(uint64_t handle, float roughness) {
        mMaterialManager.SetMaterialRoughness(handle, roughness);
        return mMaterialManager.GetMaterial(handle) != nullptr;
    }

    bool Runtime::SetMaterialMetallic(uint64_t handle, float metallic) {
        mMaterialManager.SetMaterialMetallic(handle, metallic);
        return mMaterialManager.GetMaterial(handle) != nullptr;
    }

    bool Runtime::HasMaterial(uint64_t handle) const {
        return mMaterialManager.GetMaterial(handle) != nullptr;
    }
    int Runtime::DebugLoadedMaterialsCount() const {
        return static_cast<int>(mMaterialManager.Debug_LoadedMaterialsCount());
    }
    uint64_t Runtime::CreateMeshHandle(ASMeshData* meshData) {
        if (!meshData) {
            CE::Log(CE::LogLevel::Warn, "[AngelScript 3D] CreateMeshHandle called with a null mesh");
            return 0;
        }

        return mGPUMeshManager.CreateMeshHandle(meshData->mesh);
    }

    void Runtime::ChangeMesh(uint64_t handle, ASMeshData* meshData) {
        if (!meshData) {
            CE::Log(CE::LogLevel::Warn, "[AngelScript 3D] ChangeMesh called with a null mesh");
            return;
        }

        mGPUMeshManager.ChangeMesh(handle, meshData->mesh);
    }

    uint64_t Runtime::DestroyMesh(uint64_t handle) {
        mGPUMeshManager.DestroyMesh(handle);
        return handle;
    }

    bool Runtime::HasMesh(uint64_t handle) const {
        return mGPUMeshManager.HasMesh(handle);
    }

    void Runtime::DrawMesh(uint64_t handle, const Transform3DDesc& transform, uint64_t materialHandle, bool errorTexture) {
        auto engineTransform = ToTransform(transform);
        mGPUMeshManager.DrawMeshHandle(handle, engineTransform, materialHandle, errorTexture);
    }
}
