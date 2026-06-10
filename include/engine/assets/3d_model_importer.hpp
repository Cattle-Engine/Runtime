#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
                Renderer::Resources::MaterialManager& mat_manager,
                Renderer::Resources::TextureManager& tex_man
            );

            Renderer::Resources::Model ImportModel(std::string path);
        private:
            CE::Renderer::MeshData ConvertMesh(aiMesh* mesh);
            Renderer::Resources::TextureHandle LoadAssimpMaterial(
                const aiScene* scene,
                const aiMaterial* mat,
                Renderer::Resources::Model& model
            );

            VFS::VFS& mVFS;
            Renderer::Resources::GPUMeshManager& mGPUMeshManager;
            Renderer::Resources::MaterialManager& mMaterialManager;
            Renderer::Resources::TextureManager& mTextureManager;
    };
}