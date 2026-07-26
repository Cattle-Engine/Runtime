#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL_surface.h>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace CE::Assets::Model3DImporter {
    class ModelImporter {
      public:
        ModelImporter(VFS::VFS& vfs, Renderer::Resources::GPUMeshManager& mesh_manager,
                      Renderer::Resources::MaterialManager& mat_manager, Renderer::Resources::TextureManager& tex_man,
                      Renderer::IRenderer& renderer);

        /*
         * @brief Loads a model from the VFS
         * @param path Path to the model
         * @return Returns an empty Model on failure
         */
        Renderer::Resources::Model ImportModel(std::string path);

      private:
        struct TextureInfo {
            std::string path = "";
            Renderer::Resources::TextureHandle handle{};
        };

        struct LoadedAssimpTextureInfo {
            SDL_Surface* texture = nullptr;
            TextureInfo tex_info; // Used when a texture is already loaded
            std::string cache_key = "";
        };

        SDL_Surface* DecodeSurface(const aiScene* scene, const std::string& path, const std::string& textureName,
                                   std::unordered_map<std::string, SDL_Surface*>& cache);

        CE::Renderer::MeshData ConvertMesh(aiMesh* mesh);
        SDL_Surface* BuildMR(SDL_Surface* metallic, SDL_Surface* roughness);
        LoadedAssimpTextureInfo LoadAssimpTexture(const aiScene* scene, const aiMaterial* mat, aiTextureType type,
                                                  Renderer::Resources::Model& model, std::string model_path,
                                                  std::vector<TextureInfo>& mat_io_vector);

        Renderer::Resources::MaterialHandle
        LoadAssimpMaterial(const aiScene* scene, const aiMaterial* material, Renderer::Resources::Model& model,
                           const std::string& path, std::vector<TextureInfo>& gpuHandleCache,
                           std::unordered_map<std::string, SDL_Surface*>& surfaceCache,
                           Renderer::TextureUploadBatch* batch);

        VFS::VFS& mVFS;
        Renderer::Resources::GPUMeshManager& mGPUMeshManager;
        Renderer::Resources::MaterialManager& mMaterialManager;
        Renderer::Resources::TextureManager& mTextureManager;
        // Used to batch textures
        Renderer::IRenderer& mRenderer;
    };
} // namespace CE::Assets::Model3DImporter