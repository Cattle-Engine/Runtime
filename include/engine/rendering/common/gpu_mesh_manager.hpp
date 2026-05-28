#pragma once

#include <cstdint>
#include <unordered_map>
#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Resources {
    using MeshHandle = uint32_t;

    class GPUMeshManager {
        public:
            explicit GPUMeshManager(IRenderer& renderer);
            ~GPUMeshManager();

            MeshHandle CreateMeshHandle(MeshData& data);
            void DrawMeshHandle(MeshHandle handle, const Transform3D& transform, const Material* material = nullptr, bool error_texture = false);
            void ChangeMesh(MeshHandle& handle, MeshData& data);
            void DestroyMesh(MeshHandle& handle);
            GPUMesh* GetMesh(MeshHandle handle);
            const GPUMesh* GetMesh(MeshHandle handle) const;
            bool HasMesh(MeshHandle handle) const;
        private:
            struct MeshInfo {
                GPUMesh* Mesh = nullptr;
            };
            GPUMesh* CreateMesh(MeshData& data);

            IRenderer& mRenderer;
            std::unordered_map<MeshHandle, MeshInfo> mGPUMeshes;
            MeshHandle mNextHandle = 1;
    };
}
