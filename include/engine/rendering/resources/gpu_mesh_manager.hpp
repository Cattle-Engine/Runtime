#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/material_manager.hpp"

#include <glm/glm.hpp>

namespace CE::Renderer::Resources {
    using MeshHandle = uint64_t;

    class GPUMeshManager {
      public:
        GPUMeshManager(IRenderer& renderer, MaterialManager& mat_manager);
        ~GPUMeshManager();

        MeshHandle CreateMeshHandle(MeshData& data);
        void DrawMeshHandle(MeshHandle handle, const Transform3D& transform, MaterialHandle matt_handle,
                            bool error_texture = false);
        void DrawMeshHandleMat4(MeshHandle handle, glm::mat4 transform, MaterialHandle matt_handle,
                                bool error_texture = false);
        void ChangeMesh(MeshHandle handle, MeshData& data);
        void DestroyMesh(MeshHandle handle);
        GPUMesh* GetMesh(MeshHandle handle);
        const GPUMesh* GetMesh(MeshHandle handle) const;
        bool HasMesh(MeshHandle handle) const;

      private:
        struct MeshInfo {
            GPUMesh* Mesh = nullptr;
        };
        GPUMesh* CreateMesh(MeshData& data);

        MaterialManager& mMaterialManager;
        IRenderer& mRenderer;
        std::unordered_map<MeshHandle, MeshInfo> mGPUMeshes;
        MeshHandle mNextHandle = 1;
    };
} // namespace CE::Renderer::Resources
