#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <assimp/Importer.hpp>

#include "engine/assets/3d_model_importer.hpp"
#include "engine/assets/assimp_vfs_io.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/utils/scoped_timer.hpp"

namespace {
    using kModel = CE::Renderer::Resources::Model;

    static SDL_Surface* CreateWhiteSurface() {
        SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            return 0;
        }

        Uint32* pixels = static_cast<Uint32*>(surface->pixels);
        pixels[0] = 0xFFFFFFFF;

        return surface;
    }
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

        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& f = mesh->mFaces[i];

            for (uint32_t j = 0; j < f.mNumIndices; j++) {
                out.indices.push_back(f.mIndices[j]);
            }
        }

        out.vertex_count = (uint32_t)out.vertices.size();
        out.indice_count = (uint32_t)out.indices.size();

        if (mesh->HasTangentsAndBitangents()) {
            for (uint32_t i = 0; i < mesh->mNumVertices && i < out.vertices.size(); ++i) {
                glm::vec3 tangent = glm::vec3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );

                glm::vec3 bitangent = glm::vec3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                );

                glm::vec3 n = out.vertices[i].normal;

                tangent = glm::normalize(tangent - n * glm::dot(n, tangent));

                float handedness = (glm::dot(glm::cross(n, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

                out.vertices[i].tangent = tangent;
                out.vertices[i].tangentSign = handedness;
            }
        } else {
            for (size_t i = 0; i < out.vertices.size(); i++) {
                out.vertices[i].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                out.vertices[i].tangentSign = 1.0f;
            }
        }

        return out;
    }

    SDL_Surface* ModelImporter::BuildMR(SDL_Surface* metallic, SDL_Surface* roughness) {
        int w = metallic ? metallic->w : (roughness ? roughness->w : 1);
        int h = metallic ? metallic->h : (roughness ? roughness->h : 1);

        SDL_Surface* out = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);

        SDL_LockSurface(out);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float m = 0.0f;
                float r = 1.0f;

                if (metallic) {
                    Uint8* mp = (Uint8*)((Uint8*)metallic->pixels + y * metallic->pitch + x * 4);
                    m = mp[0] / 255.0f;
                }

                if (roughness) {
                    Uint8* rp = (Uint8*)((Uint8*)roughness->pixels + y * roughness->pitch + x * 4);
                    r = rp[0] / 255.0f;
                }

                Uint8* op = (Uint8*)((Uint8*)out->pixels + y * out->pitch + x * 4);

                op[0] = 0;
                op[1] = (Uint8)(r * 255.0f);
                op[2] = (Uint8)(m * 255.0f);
                op[3] = 255;
            }
        }

        SDL_UnlockSurface(out);

        return out;
    }

    glm::mat4 ConvertMatrix(const aiMatrix4x4& m) {
        glm::mat4 result;

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

    uint32_t ConvertNode(aiNode* node, kModel& model) {
        uint32_t nodeIndex = (uint32_t)model.Nodes.size();

        model.Nodes.emplace_back();

        kModel::Node& outNode = model.Nodes[nodeIndex];

        outNode.Transform = ConvertMatrix(node->mTransformation);

        outNode.MeshIndices.reserve(node->mNumMeshes);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            outNode.MeshIndices.push_back(node->mMeshes[i]);
        }

        outNode.Children.reserve(node->mNumChildren);

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            outNode.Children.push_back(ConvertNode(node->mChildren[i], model));
        }

        return nodeIndex;
    }

    SDL_Surface* ModelImporter::LoadAssimpTexture(
        const aiScene* scene,
        const aiMaterial* mat,
        aiTextureType type,
        Renderer::Resources::Model& model,
        std::string mdl_path
    ) {
        aiString tex_path;

        if (mat->GetTexture(type, 0, &tex_path) != AI_SUCCESS) {
            return CreateWhiteSurface();
        }

        std::string key = tex_path.C_Str();

        if (auto it = mTextureCache.find(key); it != mTextureCache.end()) {
            return it->second;
        }

        Utils::ScopedTimer timer("[3D Model Loader] Material texture loading took");

        SDL_Surface* result = nullptr;

        if (!key.empty() && key[0] == '*') {
            try {
                int index = std::stoi(key.substr(1));
                const aiTexture* tex = scene->mTextures[index];

                if (tex->mHeight == 0) {
                    SDL_IOStream* rw = SDL_IOFromConstMem(tex->pcData, (size_t)tex->mWidth);
                    result = IMG_Load_IO(rw, 1);
                } else {
                    result = SDL_CreateSurfaceFrom(
                        tex->mWidth,
                        tex->mHeight,
                        SDL_PIXELFORMAT_BGRA32,
                        tex->pcData,
                        tex->mWidth * 4
                    );
                }

                if (result && result->format != SDL_PIXELFORMAT_RGBA32) {
                    SDL_Surface* converted = SDL_ConvertSurface(result, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(result);
                    result = converted;
                }
            } catch (...) {
                result = CreateWhiteSurface();
            }
        } else {
            std::filesystem::path combined = std::filesystem::path(mdl_path).parent_path() / key;

            std::string vpath = mVFS.NormalizeVirtualPath(combined.generic_string());

            if (mVFS.FileExists(vpath.c_str())) {
                VirtualFile* file = mVFS.OpenFile(vpath.c_str());

                result = IMG_Load_IO(file->sdl_stream, false);

                if (result && result->format != SDL_PIXELFORMAT_RGBA32) {
                    SDL_Surface* converted = SDL_ConvertSurface(result, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(result);
                    result = converted;
                }
            }
        }

        if (!result) {
            result = CreateWhiteSurface();
        }

        mTextureCache[key] = result;

        return result;
    }

    Renderer::Resources::MaterialHandle ModelImporter::LoadAssimpMaterial(
        const aiScene* scene,
        const aiMaterial* mat,
        Renderer::Resources::Model& model,
        std::string model_path
    ) {
        SDL_Surface* albedo = LoadAssimpTexture(scene, mat, aiTextureType_DIFFUSE, model, model_path);
        SDL_Surface* normal = LoadAssimpTexture(scene, mat, aiTextureType_NORMALS, model, model_path);
        SDL_Surface* rough = LoadAssimpTexture(scene, mat, aiTextureType_DIFFUSE_ROUGHNESS, model, model_path);
        SDL_Surface* metal = LoadAssimpTexture(scene, mat, aiTextureType_METALNESS, model, model_path);

        Utils::ScopedTimer t1("[3D Model Loader] Material texture GPU upload");

        auto albedo_h = mTextureManager.CreateTextureFromData(
            albedo->w,
            albedo->h,
            albedo->pixels,
            Renderer::TextureFormat::RGBA8,
            albedo->pitch,
            Renderer::TextureFilter::Linear,
            Renderer::TextureWrap::Repeat
        );

        auto normal_h = mTextureManager.CreateTextureFromData(
            normal->w,
            normal->h,
            normal->pixels,
            Renderer::TextureFormat::RGBA8,
            normal->pitch,
            Renderer::TextureFilter::Linear,
            Renderer::TextureWrap::Repeat
        );

        SDL_Surface* mr = BuildMR(metal, rough);

        auto mr_h = mTextureManager.CreateTextureFromData(
            mr->w,
            mr->h,
            mr->pixels,
            Renderer::TextureFormat::RGBA8,
            mr->pitch,
            Renderer::TextureFilter::Linear,
            Renderer::TextureWrap::Repeat
        );

        SDL_DestroySurface(mr);

        Utils::ScopedTimer t2("[3D Model Importer] Total time to create material and set fields");

        Renderer::Resources::MaterialHandle mat_h = mMaterialManager.CreateMaterial(albedo_h);

        mMaterialManager.SetNormalTexture(mat_h, normal_h);
        mMaterialManager.SetMetallicRoughnessTexture(mat_h, mr_h);

        float metallic;
        float roughness;

        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

        mMaterialManager.SetMaterialMetallic(mat_h, metallic);
        mMaterialManager.SetMaterialRoughness(mat_h, roughness);

        return mat_h;
    }

    Renderer::Resources::Model ModelImporter::ImportModel(std::string path) {
        Utils::ScopedTimer total("[3D Model Importer] Total import time");

        mTextureCache.clear();

        kModel model;

        if (!mVFS.FileExists(path.c_str())) {
            return model;
        }

        Assimp::Importer importer;

        importer.SetIOHandler(new CE::Assets::VFSIOSystem(&mVFS));

        const aiScene* scene;

        {
            Utils::ScopedTimer t("[3D Model Importer] Assimp file read");
            scene = importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_FlipUVs |
                aiProcess_JoinIdenticalVertices |
                aiProcess_SortByPType
            );
        }

        if (!scene || !scene->mRootNode) {
            return model;
        }

        model.Materials.reserve(scene->mNumMaterials);

        std::vector<CE::Renderer::MeshData> mesh_data;

        {
            Utils::ScopedTimer t("[3D Model Importer] Mesh conversion");

            mesh_data.reserve(scene->mNumMeshes);

            for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
                mesh_data.push_back(ConvertMesh(scene->mMeshes[i]));
                model.MeshMaterialIndices.push_back(scene->mMeshes[i]->mMaterialIndex);
            }
        }

        {
            Utils::ScopedTimer t("[3D Model Importer] Material loading");

            for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
                model.Materials.push_back(
                    LoadAssimpMaterial(scene, scene->mMaterials[i], model, path)
                );
            }
        }

        {
            Utils::ScopedTimer t("[3D Model Importer] GPU mesh upload");

            for (auto& mesh : mesh_data) {
                model.Meshes.push_back(mGPUMeshManager.CreateMeshHandle(mesh));
            }
        }

        model.RootNode = ConvertNode(scene->mRootNode, model);

        return model;
    }
}