#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Assets::Model3DImporter {    
    class ModelImporter {
        public:
            ModelImporter(
                VFS::VFS& vfs,
                Renderer::Resources::GPUMeshManager& mesh_manager,
                Renderer::Resources::MaterialManager& mat_manager
            );

            Renderer::Resources::Model ImportModel(std::string path);
        private:
            VFS::VFS& mVFS;
            Renderer::Resources::GPUMeshManager& mGPUMeshManager;
            Renderer::Resources::MaterialManager& mMaterialManager;
    };
}