#pragma once

#include <cstdint>
#include <unordered_map>
#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Resources {
    using MeshHandle = uint32_t;
    
    class GPUMeshManager {
        public:
            GPUMeshManager(IRenderer& renderer);
            MeshHandle CreateMeshHandle(MeshData& data);
            void DrawMeshHandle(const MeshHandle handle, Texture* tex, bool error_texture);
            void ChangeMesh(MeshHandle& handle, MeshData& data);
            void DestroyMesh(MeshHandle& handle);
            void GetMesh(MeshHandle& handle);
        private:
            std::unordered_map<MeshHandle, GPUMesh> mGPUMeshes;
            MeshHandle mNextHandle;
    };
}