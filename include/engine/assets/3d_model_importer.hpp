#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <SDL3/SDL_surface.h>

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

            /*
            * @brief Loads a model from the VFS
            * @param path Path to the model
            * @return Returns an empty Model on failure  
            */
            Renderer::Resources::Model ImportModel(std::string path);
        private:
            CE::Renderer::MeshData ConvertMesh(aiMesh* mesh);
            SDL_Surface* BuildMR(SDL_Surface* metallic, SDL_Surface* roughness);
            SDL_Surface* LoadAssimpTexture(
                const aiScene* scene,
                const aiMaterial* mat,
                aiTextureType type,
                Renderer::Resources::Model& model,
                std::string model_path
            );

            Renderer::Resources::MaterialHandle LoadAssimpMaterial(
                const aiScene* scene,
                const aiMaterial* mat,
                Renderer::Resources::Model& model,
                std::string model_path        
            );

            VFS::VFS& mVFS;
            Renderer::Resources::GPUMeshManager& mGPUMeshManager;
            Renderer::Resources::MaterialManager& mMaterialManager;
            Renderer::Resources::TextureManager& mTextureManager;
    };
}