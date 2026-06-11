#include <string>
#include <filesystem>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <assimp/Importer.hpp>

#include "engine/assets/3d_model_importer.hpp"
#include "engine/assets/assimp_vfs_io.hpp"
#include "engine/common/tracelog.hpp"

namespace {
    using kModel = CE::Renderer::Resources::Model;
}
namespace CE::Assets::Model3DImporter {
    ModelImporter::ModelImporter(
        VFS::VFS& vfs,
        Renderer::Resources::GPUMeshManager& mesh_manager,
        Renderer::Resources::MaterialManager& mat_manager,
        Renderer::Resources::TextureManager& tex_man
    ) : mVFS(vfs), mGPUMeshManager(mesh_manager), mMaterialManager(mat_manager), mTextureManager(tex_man) {}

    CE::Renderer::MeshData ModelImporter::ConvertMesh(aiMesh* mesh) {
        CE::Renderer::MeshData out;

        out.vertices.reserve(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            CE::Renderer::Vertex3D v;

            v.position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            v.normal = mesh->HasNormals()
                ? glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                )
                : glm::vec3(0.0f);

            if (mesh->HasTextureCoords(0)) {
                v.uv = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            } else {
                v.uv = glm::vec2(0.0f);
            }

            v.color = {255, 255, 255, 255};

            out.vertices.push_back(v);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& f = mesh->mFaces[i];

            for (uint32_t j = 0; j < f.mNumIndices; j++)
                out.indices.push_back(f.mIndices[j]);
        }

        out.vertex_count = (uint32_t)out.vertices.size();
        out.indice_count = (uint32_t)out.indices.size();

        return out;
    }

    glm::mat4 ConvertMatrix( const aiMatrix4x4& m) {
        glm::mat4 result;
        
        // This was a fricking nightmare
        result[0][0] = m.a1;
        result[1][0] = m.a2;
        result[2][0] = m.a3;
        result[3][0] = m.a4;

        result[0][1] = m.b1;
        result[1][1] = m.b2;
        result[2][1] = m.b3;
        result[3][1] = m.b4;

        result[0][2] = m.c1;
        result[1][2] = m.c2;
        result[2][2] = m.c3;
        result[3][2] = m.c4;

        result[0][3] = m.d1;
        result[1][3] = m.d2;
        result[2][3] = m.d3;
        result[3][3] = m.d4;

        return result;
    }

    uint32_t ConvertNode(
        aiNode* node,
        kModel& model
    ) {
        uint32_t nodeIndex =
            static_cast<uint32_t>(model.Nodes.size());

        model.Nodes.emplace_back();

        kModel::Node& outNode =
            model.Nodes[nodeIndex];

        outNode.Transform =
            ConvertMatrix(node->mTransformation);

        outNode.MeshIndices.reserve(node->mNumMeshes);

        for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
            outNode.MeshIndices.push_back(
                node->mMeshes[i]
            );
        }

        outNode.Children.reserve(node->mNumChildren);

        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            uint32_t childIndex =
                ConvertNode(
                    node->mChildren[i],
                    model
                );

            outNode.Children.push_back(childIndex);
        }

        return nodeIndex;
    }

    Renderer::Resources::TextureHandle ModelImporter::LoadAssimpTexture(
        const aiScene* scene,
        const aiMaterial* mat,
        Renderer::Resources::Model& model,
        std::string mdl_path
    ) { 
        aiString tex_path;

        if(mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex_path) == AI_SUCCESS) {
            std::string path = tex_path.C_Str();
            size_t path_string_pos;

            if (!path.empty() && path[0] == '*') {
                try {   
                    int index = std::stoi(path.substr(1), &path_string_pos);
                    const aiTexture* tex = scene->mTextures[index];

                    if (tex->mHeight == 0) {
                        const unsigned char* data = reinterpret_cast<const unsigned char*>(tex->pcData);
                        int size = static_cast<int>(tex->mWidth);
                        SDL_IOStream* rw = SDL_IOFromConstMem(data, size);
                        if (!rw) return 0;

                        SDL_Surface* surface = IMG_Load_IO(rw, 1);
                        SDL_Surface* formated_texture;
                        
                        if (!surface) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to create SDL_Surface");
                            SDL_DestroySurface(surface);
                            return 0;
                        }

                        formated_texture = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);

                        if (!formated_texture) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
                            SDL_DestroySurface(surface);
                            SDL_DestroySurface(formated_texture);
                            return 0;
                        }

                        auto handle = mTextureManager.CreateTextureFromData(
                            formated_texture->w,
                            formated_texture->h,
                            formated_texture->pixels,
                            Renderer::TextureFormat::RGBA8,
                            formated_texture->pitch
                        );
                        SDL_DestroySurface(surface);
                        SDL_DestroySurface(formated_texture);

                        return handle;
                    } else {
                        SDL_Surface* formated_surface; 
                        SDL_Surface* surface = SDL_CreateSurfaceFrom(
                            tex->mWidth,
                            tex->mHeight,
                            SDL_PIXELFORMAT_BGRA32,
                            tex->pcData,
                            tex->mWidth * 4
                        );
                        
                        if (!surface) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to create SDL_Surface");
                            SDL_DestroySurface(surface);
                            return 0;
                        }

                        formated_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);

                        if (!formated_surface) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
                            SDL_DestroySurface(surface);
                            SDL_DestroySurface(formated_surface);
                            return 0;
                        }

                        auto handle = mTextureManager.CreateTextureFromData(
                            formated_surface->w,
                            formated_surface->h,
                            formated_surface->pixels,
                            Renderer::TextureFormat::RGBA8,
                            formated_surface->pitch
                        );

                        SDL_DestroySurface(surface);
                        SDL_DestroySurface(formated_surface);
                        return handle;
                    }
                } catch (const std::invalid_argument&) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] std::stoi threw invalid argument at position: {}", path_string_pos);
                    return 0;
                } catch (const std::out_of_range&) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] std::stoi throw out of range");
                    return 0;
                }
            } else {
                std::filesystem::path base(mdl_path);
                std::filesystem::path combined = base / path;

                std::string virtual_path = combined.generic_string();
                virtual_path = mVFS.NormalizeVirtualPath(virtual_path);

                return mTextureManager.Load(virtual_path);
            }
        }
    }


    Renderer::Resources::MaterialHandle ModelImporter::LoadAssimpMaterial(
        const aiScene* scene,
        const aiMaterial* mat,
        Renderer::Resources::Model& model,
        std::string model_path
    ) {
        auto tex = LoadAssimpTexture(scene, mat, model, model_path);

        Renderer::Resources::MaterialHandle material = mMaterialManager.CreateMaterial(tex);

        float metallic;
        float roughness;

        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    
        mMaterialManager.SetMaterialMetallic(material ,metallic);
        mMaterialManager.SetMaterialRoughness(material ,roughness);
        return material;
    }

    Renderer::Resources::Model ModelImporter::ImportModel(std::string path) {
        kModel model;
        if (!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error, "[3D Model Importer] File: {}, doesn't exist", path);
            return model;
        }

        Assimp::Importer importer;

        importer.SetIOHandler(
            new CE::Assets::VFSIOSystem(&mVFS)
        );

        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | aiProcess_GenNormals
        );

        if (!scene || !scene->mRootNode) {
            CE::Log(
                LogLevel::Error,
                "[3D Model Importer] Assimp error: {}",
                importer.GetErrorString()
            );

            return model;
        }

        model.Materials.reserve(scene->mNumMaterials);
        model.MeshesCPU.reserve(scene->mNumMeshes);
        model.MeshMaterialIndices.reserve(scene->mNumMeshes);
        
        for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[i];

            model.MeshesCPU.push_back(ConvertMesh(mesh));

            model.MeshMaterialIndices.push_back(mesh->mMaterialIndex);
        }

        for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* mat = scene->mMaterials[i];

            model.Materials.push_back(
                LoadAssimpMaterial(scene, mat, model, path)
            );
        }

        model.Meshes.reserve(scene->mNumMeshes);
        for (auto& mesh : model.MeshesCPU) {
            model.Meshes.push_back(
                mGPUMeshManager.CreateMeshHandle(mesh)
            );
        }
        
        model.RootNode = ConvertNode(
            scene->mRootNode,
            model
        );

        return model;
    }
}