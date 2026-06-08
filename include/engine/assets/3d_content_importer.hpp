#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/common/texture_manager.hpp"
#include "engine/rendering/common/material_manager.hpp"
#include "engine/rendering/common/gpu_mesh_manager.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Assets::Content3DImporters {
    struct ModelAsset {
        struct Node {
            Renderer::Transform3D Transform;
            std::vector<uint32_t> MeshIndices;
            std::vector<uint32_t> Children;
        };

        std::vector<Renderer::Resources::MeshHandle> Meshes;
        std::vector<Renderer::Resources::MaterialHandle> Materials;
        std::vector<Node> Nodes;

        uint32_t RootNode = 0;
    };
    
    class ModelImporter {
        public:
            ModelImporter(
                VFS::VFS& vfs,
                Renderer::Resources::GPUMeshManager& mesh_manager,
                Renderer::Resources::MaterialManager& mat_manager
            );
        private:
            VFS::VFS& mVFS;
            Renderer::Resources::GPUMeshManager& mGPUMeshManager;
            Renderer::Resources::MaterialManager& mMaterialManager;
    };
}