#include "engine/scripting/angelscript.hpp"

#include "engine/common/tracelog.hpp"
#include "engine/rendering/common/gpu_mesh_manager.hpp"
#include "engine/rendering/common/material_manager.hpp"

#include <new>

namespace {
    constexpr const char* k3DNamespace = "CE::Graphics::ThreeD";
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
            "void LoadSkyBox(const string &in name, const string &in frontPath, const string &in backPath, const string &in leftPath, const string &in rightPath)",
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
            "void LoadMaterial(const string &in name)",
            asMETHODPR(Runtime, LoadMaterial, (const std::string&), void),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void LoadMaterial(const string &in name, const Material &in material)",
            asMETHODPR(Runtime, LoadMaterial, (const std::string&, const MaterialDesc&), void),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadMaterial(const string &in name)",
            asMETHOD(Runtime, UnloadMaterial),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialAlbedo(const string &in name, const string &in textureName)",
            asMETHOD(Runtime, SetMaterialAlbedo),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialTint(const string &in name, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, SetMaterialTint),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialRoughness(const string &in name, float roughness)",
            asMETHOD(Runtime, SetMaterialRoughness),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetMaterialMetallic(const string &in name, float metallic)",
            asMETHOD(Runtime, SetMaterialMetallic),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool HasMaterial(const string &in name)",
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
            "uint CreateMeshHandle(CE::Graphics::MeshData@ meshData)",
            asMETHOD(Runtime, CreateMeshHandle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void ChangeMesh(uint handle, CE::Graphics::MeshData@ meshData)",
            asMETHOD(Runtime, ChangeMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "uint DestroyMesh(uint handle)",
            asMETHOD(Runtime, DestroyMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool HasMesh(uint handle)",
            asMETHOD(Runtime, HasMesh),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawMesh(uint handle, const Transform3D &in transform, const string &in materialName = \"\", bool errorTexture = false)",
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
        const std::string& rightPath
    ) {
        mSkyboxManager.Load(
            frontPath.c_str(),
            backPath.c_str(),
            leftPath.c_str(),
            rightPath.c_str(),
            name.c_str()
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

    void Runtime::LoadMaterial(const std::string& name) {
        mMaterialManager.Load(name.c_str());
    }

    void Runtime::LoadMaterial(const std::string& name, const MaterialDesc& material) {
        Renderer::Material engineMaterial {};
        engineMaterial.tint = material.tint;
        engineMaterial.roughness = material.roughness;
        engineMaterial.metallic = material.metallic;
        mMaterialManager.Load(name.c_str(), engineMaterial);
    }

    void Runtime::UnloadMaterial(const std::string& name) {
        mMaterialManager.Unload(name.c_str());
    }

    bool Runtime::SetMaterialAlbedo(const std::string& name, const std::string& textureName) {
        return mMaterialManager.SetAlbedo(name.c_str(), textureName.c_str());
    }

    bool Runtime::SetMaterialTint(const std::string& name, const Renderer::Colour& colour) {
        return mMaterialManager.SetTint(name.c_str(), colour);
    }

    bool Runtime::SetMaterialRoughness(const std::string& name, float roughness) {
        return mMaterialManager.SetRoughness(name.c_str(), roughness);
    }

    bool Runtime::SetMaterialMetallic(const std::string& name, float metallic) {
        return mMaterialManager.SetMetallic(name.c_str(), metallic);
    }

    bool Runtime::HasMaterial(const std::string& name) const {
        return mMaterialManager.Has(name.c_str());
    }

    int Runtime::DebugLoadedMaterialsCount() const {
        return mMaterialManager.Debug_LoadedMaterialsCount();
    }

    uint32_t Runtime::CreateMeshHandle(ASMeshData* meshData) {
        if (!meshData) {
            CE::Log(CE::LogLevel::Warn, "[AngelScript 3D] CreateMeshHandle called with a null mesh");
            return 0;
        }

        return mGPUMeshManager.CreateMeshHandle(meshData->mesh);
    }

    void Runtime::ChangeMesh(uint32_t handle, ASMeshData* meshData) {
        if (!meshData) {
            CE::Log(CE::LogLevel::Warn, "[AngelScript 3D] ChangeMesh called with a null mesh");
            return;
        }

        auto meshHandle = handle;
        mGPUMeshManager.ChangeMesh(meshHandle, meshData->mesh);
    }

    uint32_t Runtime::DestroyMesh(uint32_t handle) {
        mGPUMeshManager.DestroyMesh(handle);
        return handle;
    }

    bool Runtime::HasMesh(uint32_t handle) const {
        return mGPUMeshManager.HasMesh(handle);
    }

    void Runtime::DrawMesh(uint32_t handle, const Transform3DDesc& transform, const std::string& materialName, bool errorTexture) {
        const Renderer::Material* material = nullptr;
        if (!materialName.empty()) {
            material = mMaterialManager.Get(materialName.c_str());
        }

        auto engineTransform = ToTransform(transform);
        mGPUMeshManager.DrawMeshHandle(handle, engineTransform, material, errorTexture);
    }
}
