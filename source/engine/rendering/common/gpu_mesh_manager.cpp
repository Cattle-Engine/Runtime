#include "engine/rendering/common/gpu_mesh_manager.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    GPUMeshManager::GPUMeshManager(IRenderer& renderer, MaterialManager& mat_manager) :
        mRenderer(renderer), mMaterialManager(mat_manager) {
    }

    GPUMeshManager::~GPUMeshManager() {
        for (auto& [handle, meshInfo] : mGPUMeshes) {
            (void)handle;
            if (meshInfo.Mesh) {
                mRenderer.DestroyGPUMesh(meshInfo.Mesh);
                meshInfo.Mesh = nullptr;
            }
        }
        mGPUMeshes.clear();
    }

    GPUMesh* GPUMeshManager::CreateMesh(MeshData& data) {
        return mRenderer.CreateGPUMesh(data);
    }

    MeshHandle GPUMeshManager::CreateMeshHandle(MeshData& data) {
        GPUMesh* mesh = CreateMesh(data);
        if (!mesh) {
            CE::Log(LogLevel::Error, "[GPUMesh Manager] Failed to create mesh");
            return 0;
        }

        const MeshHandle handle = mNextHandle++;
        MeshInfo meshInfo{};
        meshInfo.Mesh = mesh;
        mGPUMeshes[handle] = meshInfo;
        return handle;
    }

    void GPUMeshManager::DrawMeshHandle(MeshHandle handle, const Transform3D& transform, MaterialHandle matt_handle, bool error_texture = false) {
        auto it = mGPUMeshes.find(handle);
        if (it == mGPUMeshes.end() || !it->second.Mesh) {
            CE::Log(LogLevel::Warn, "[GPUMesh Manager] Tried to draw missing mesh handle {}", handle);
            return;
        }

        Material material_to_draw = {};

        auto material_from_manager = mMaterialManager.GetMaterial(matt_handle);

        if (material_from_manager) {
            material_to_draw = *material_from_manager;
        }

        mRenderer.DrawMesh(
            it->second.Mesh,
            material_to_draw,
            transform,
            error_texture
        );
    }

    void GPUMeshManager::ChangeMesh(MeshHandle& handle, MeshData& data) {
        auto it = mGPUMeshes.find(handle);
        if (it == mGPUMeshes.end()) {
            CE::Log(LogLevel::Warn, "[GPUMesh Manager] Tried to change missing mesh handle {}", handle);
            return;
        }

        GPUMesh* newMesh = CreateMesh(data);
        if (!newMesh) {
            CE::Log(LogLevel::Error, "[GPUMesh Manager] Failed to rebuild mesh {}", handle);
            return;
        }

        if (it->second.Mesh) {
            mRenderer.DestroyGPUMesh(it->second.Mesh);
        }
        it->second.Mesh = newMesh;
    }

    void GPUMeshManager::DestroyMesh(MeshHandle& handle) {
        auto it = mGPUMeshes.find(handle);
        if (it == mGPUMeshes.end()) {
            handle = 0;
            return;
        }

        if (it->second.Mesh) {
            mRenderer.DestroyGPUMesh(it->second.Mesh);
            it->second.Mesh = nullptr;
        }

        mGPUMeshes.erase(it);
        handle = 0;
    }

    GPUMesh* GPUMeshManager::GetMesh(MeshHandle handle) {
        auto it = mGPUMeshes.find(handle);
        if (it == mGPUMeshes.end()) {
            return nullptr;
        }
        return it->second.Mesh;
    }

    const GPUMesh* GPUMeshManager::GetMesh(MeshHandle handle) const {
        auto it = mGPUMeshes.find(handle);
        if (it == mGPUMeshes.end()) {
            return nullptr;
        }
        return it->second.Mesh;
    }

    bool GPUMeshManager::HasMesh(MeshHandle handle) const {
        return mGPUMeshes.find(handle) != mGPUMeshes.end();
    }
}
