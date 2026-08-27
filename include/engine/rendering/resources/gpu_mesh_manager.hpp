#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/material_manager.hpp"

#include <glm/glm.hpp>

namespace CE::Renderer::Resources {
    struct MeshHandle {
        MeshHandle() : id(0) {}
        MeshHandle(const MeshHandle& other) : id(other.id) {}

        uint64_t id = 0;

        explicit operator bool() const {
            return id != 0;
        }

        bool operator==(const MeshHandle& other) const {
            return id == other.id;
        }
    };

    struct MeshHandleHash {
        size_t operator()(const MeshHandle& handle) const {
            return std::hash<uint64_t>{}(handle.id);
        }
    };

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
        std::unordered_map<MeshHandle, MeshInfo, MeshHandleHash> mGPUMeshes;
        uint64_t mNextHandle = 1;
    };
} // namespace CE::Renderer::Resources
