#include "engine/assets/3d_model_importer.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "engine/assets/assimp_vfs_io.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/utils/scoped_timer.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

#include <SDL3_image/SDL_image.h>
#include <assimp/Importer.hpp>

namespace {
    using kModel = CE::Renderer::Resources::Model;

    static SDL_Surface* CreateWhiteSurface() {
        SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            return nullptr;
        }
        Uint32* pixels = static_cast<Uint32*>(surface->pixels);
        pixels[0] = 0xFFFFFFFF;
        return surface;
    }

    static SDL_Surface* DecodeCompressedTexture(const unsigned char* data, int size) {
        SDL_IOStream* rw = SDL_IOFromConstMem(data, size);
        if (!rw) {
            return CreateWhiteSurface();
        }
        SDL_Surface* surface = IMG_Load_IO(rw, 1);
        if (!surface) {
            CE_LOG(CE::LogLevel::Error, "[3D Model Importer] IMG_Load_IO failed");
            return CreateWhiteSurface();
        }
        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE_LOG(CE::LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
            return CreateWhiteSurface();
        }
        return converted;
    }

    static SDL_Surface* DecodeRawTexture(const aiTexture* tex) {
        SDL_Surface* surface =
            SDL_CreateSurfaceFrom(tex->mWidth, tex->mHeight, SDL_PIXELFORMAT_BGRA32, tex->pcData, tex->mWidth * 4);
        if (!surface) {
            CE_LOG(CE::LogLevel::Error, "[3D Model Importer] Failed to create SDL_Surface from raw texture");
            return CreateWhiteSurface();
        }
        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE_LOG(CE::LogLevel::Error, "[3D Model Importer] Failed to convert raw surface to RGBA32");
            return CreateWhiteSurface();
        }
        return converted;
    }
} // namespace

namespace CE::Assets::Model3DImporter {

    ModelImporter::ModelImporter(VFS::VFS& vfs, Renderer::Resources::GPUMeshManager& mesh_manager,
                                 Renderer::Resources::MaterialManager& mat_manager,
                                 Renderer::Resources::TextureManager& tex_man, Renderer::IRenderer& renderer)
        : mVFS(vfs),
          mGPUMeshManager(mesh_manager),
          mMaterialManager(mat_manager),
          mTextureManager(tex_man),
          mRenderer(renderer) {}

    CE::Renderer::MeshData ModelImporter::ConvertMesh(aiMesh* mesh) {
        CE::Renderer::MeshData out;
        out.vertices.reserve(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            CE::Renderer::Vertex3D v;
            v.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            v.normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                                          : glm::vec3(0.0f);
            v.uv = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                                             : glm::vec2(0.0f);
            v.colour = {255, 255, 255, 255};
            out.vertices.push_back(v);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& f = mesh->mFaces[i];
            for (uint32_t j = 0; j < f.mNumIndices; j++)
                out.indices.push_back(f.mIndices[j]);
        }

        out.vertex_count = (uint32_t)out.vertices.size();
        out.indice_count = (uint32_t)out.indices.size();

        if (mesh->HasTangentsAndBitangents()) {
            for (uint32_t i = 0; i < mesh->mNumVertices && i < out.vertices.size(); ++i) {
                glm::vec3 tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                glm::vec3 bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                glm::vec3 n = out.vertices[i].normal;
                tangent = glm::normalize(tangent - n * glm::dot(n, tangent));
                float handedness = (glm::dot(glm::cross(n, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;
                out.vertices[i].tangent = tangent;
                out.vertices[i].tangentSign = handedness;
            }
        } else if (mesh->HasTextureCoords(0)) {
            std::vector<glm::vec3> tan1(out.vertices.size(), glm::vec3(0.0f));
            std::vector<glm::vec3> tan2(out.vertices.size(), glm::vec3(0.0f));

            for (size_t f = 0; f + 2 < out.indices.size(); f += 3) {
                uint32_t i1 = out.indices[f], i2 = out.indices[f + 1], i3 = out.indices[f + 2];
                const glm::vec3& v1 = out.vertices[i1].position;
                const glm::vec3& v2 = out.vertices[i2].position;
                const glm::vec3& v3 = out.vertices[i3].position;
                const glm::vec2& w1 = out.vertices[i1].uv;
                const glm::vec2& w2 = out.vertices[i2].uv;
                const glm::vec2& w3 = out.vertices[i3].uv;

                float x1 = v2.x - v1.x, x2 = v3.x - v1.x;
                float y1 = v2.y - v1.y, y2 = v3.y - v1.y;
                float z1 = v2.z - v1.z, z2 = v3.z - v1.z;
                float s1 = w2.x - w1.x, s2 = w3.x - w1.x;
                float t1 = w2.y - w1.y, t2 = w3.y - w1.y;
                float denom = (s1 * t2 - s2 * t1);
                float r = denom == 0.0f ? 0.0f : 1.0f / denom;

                glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
                glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

                tan1[i1] += sdir;
                tan1[i2] += sdir;
                tan1[i3] += sdir;
                tan2[i1] += tdir;
                tan2[i2] += tdir;
                tan2[i3] += tdir;
            }

            for (size_t i = 0; i < out.vertices.size(); ++i) {
                const glm::vec3& n = out.vertices[i].normal;
                glm::vec3 t = tan1[i];
                glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));
                float handedness = (glm::dot(glm::cross(n, tangent), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
                if (!(std::isfinite(tangent.x) && std::isfinite(tangent.y) && std::isfinite(tangent.z)) ||
                    glm::dot(tangent, tangent) < 1e-8f) {
                    tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                    handedness = 1.0f;
                }
                out.vertices[i].tangent = tangent;
                out.vertices[i].tangentSign = handedness;
            }
        } else {
            for (size_t i = 0; i < out.vertices.size(); ++i) {
                out.vertices[i].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                out.vertices[i].tangentSign = 1.0f;
            }
        }

        return out;
    }

    SDL_Surface* ModelImporter::BuildMR(SDL_Surface* metallic, SDL_Surface* roughness) {
        int w = metallic ? metallic->w : roughness->w;
        int h = metallic ? metallic->h : roughness->h;
        SDL_Surface* out = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        SDL_LockSurface(out);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float m = 0.0f, r = 1.0f;
                if (metallic) {
                    Uint8* mp = (Uint8*)metallic->pixels + y * metallic->pitch + x * 4;
                    m = mp[0] / 255.0f;
                }
                if (roughness) {
                    Uint8* rp = (Uint8*)roughness->pixels + y * roughness->pitch + x * 4;
                    r = rp[0] / 255.0f;
                }
                Uint8* op = (Uint8*)out->pixels + y * out->pitch + x * 4;
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
        uint32_t nodeIndex = static_cast<uint32_t>(model.Nodes.size());
        model.Nodes.emplace_back();
        kModel::Node& outNode = model.Nodes[nodeIndex];
        outNode.Transform = ConvertMatrix(node->mTransformation);
        outNode.MeshIndices.reserve(node->mNumMeshes);
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
            outNode.MeshIndices.push_back(node->mMeshes[i]);
        outNode.Children.reserve(node->mNumChildren);
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            outNode.Children.push_back(ConvertNode(node->mChildren[i], model));
        return nodeIndex;
    }

    SDL_Surface* ModelImporter::DecodeSurface(const aiScene* scene, const std::string& assimp_path,
                                              const std::string& mdl_path,
                                              std::unordered_map<std::string, SDL_Surface*>& surfaceCache) {
        auto cacheIt = surfaceCache.find(assimp_path);
        if (cacheIt != surfaceCache.end()) {
            return cacheIt->second;
        }

        SDL_Surface* result = nullptr;

        if (!assimp_path.empty() && assimp_path[0] == '*') {
            size_t path_string_pos = 0;
            try {
                int index = std::stoi(assimp_path.substr(1), &path_string_pos);
                const aiTexture* tex = scene->mTextures[index];
                if (tex->mHeight == 0) {
                    result = DecodeCompressedTexture(reinterpret_cast<const unsigned char*>(tex->pcData),
                                                     static_cast<int>(tex->mWidth));
                } else {
                    result = DecodeRawTexture(tex);
                }
            } catch (const std::invalid_argument&) {
                CE_LOG(LogLevel::Error, "[3D Model Importer] std::stoi invalid argument for path: {}", assimp_path);
                result = CreateWhiteSurface();
            } catch (const std::out_of_range&) {
                CE_LOG(LogLevel::Error, "[3D Model Importer] std::stoi out of range for path: {}", assimp_path);
                result = CreateWhiteSurface();
            }
        } else {
            std::filesystem::path base(mdl_path);
            std::filesystem::path combined = base / assimp_path;
            std::string virtual_path = mVFS.NormalizeVirtualPath(combined.generic_string());

            if (!mVFS.FileExists(virtual_path.c_str())) {
                CE_LOG(LogLevel::Error, "[3D Model Importer] Texture doesn't exist: {}", virtual_path);
                result = CreateWhiteSurface();
            } else {
                VirtualFile* file = mVFS.OpenFile(virtual_path.c_str());
                SDL_Surface* surface = IMG_Load_IO(file->sdl_stream, false);
                if (!surface) {
                    CE_LOG(LogLevel::Error, "[3D Model Importer] Failed to load texture: {}", virtual_path);
                    result = CreateWhiteSurface();
                } else if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(surface);
                    result = converted ? converted : CreateWhiteSurface();
                } else {
                    result = surface;
                }
            }
        }

        surfaceCache[assimp_path] = result;
        return result;
    }

    Renderer::Resources::MaterialHandle ModelImporter::LoadAssimpMaterial(
        const aiScene* scene, const aiMaterial* mat, [[maybe_unused]] Renderer::Resources::Model& model,
        const std::string& model_path, std::vector<TextureInfo>& gpuHandleCache,
        std::unordered_map<std::string, SDL_Surface*>& surfaceCache, Renderer::TextureUploadBatch* batch) {
        auto getTexPath = [&](aiTextureType type) -> std::string {
            aiString tex_path;
            if (mat->GetTexture(type, 0, &tex_path) == AI_SUCCESS)
                return tex_path.C_Str();
            return {};
        };

        auto resolveHandle = [&](const std::string& assimp_path,
                                 SDL_Surface* surface) -> Renderer::Resources::TextureHandle {
            if (assimp_path.empty())
                return Renderer::Resources::TextureHandle{0};

            for (const auto& entry : gpuHandleCache) {
                if (entry.path == assimp_path)
                    return entry.handle;
            }

            if (!surface)
                return Renderer::Resources::TextureHandle{0};

            auto handle = mTextureManager.CreateTextureFromData(
                surface->w, surface->h, surface->pixels, Renderer::TextureFormat::RGBA8, surface->pitch,
                Renderer::TextureFilter::Linear, Renderer::TextureWrap::Repeat, assimp_path, batch);
            gpuHandleCache.push_back({assimp_path, handle});
            return handle;
        };

        std::string albedo_path = getTexPath(aiTextureType_DIFFUSE);
        std::string normal_path = getTexPath(aiTextureType_NORMALS);
        std::string roughness_path = getTexPath(aiTextureType_DIFFUSE_ROUGHNESS);
        std::string metallic_path = getTexPath(aiTextureType_METALNESS);

        SDL_Surface* albedo_surf =
            albedo_path.empty() ? CreateWhiteSurface() : DecodeSurface(scene, albedo_path, model_path, surfaceCache);
        SDL_Surface* normal_surf =
            normal_path.empty() ? CreateWhiteSurface() : DecodeSurface(scene, normal_path, model_path, surfaceCache);
        SDL_Surface* roughness_surf = roughness_path.empty()
                                          ? CreateWhiteSurface()
                                          : DecodeSurface(scene, roughness_path, model_path, surfaceCache);
        SDL_Surface* metallic_surf = metallic_path.empty()
                                         ? CreateWhiteSurface()
                                         : DecodeSurface(scene, metallic_path, model_path, surfaceCache);

        Renderer::Resources::TextureHandle albedo_handle =
            resolveHandle(albedo_path.empty() ? "__white_albedo" : albedo_path, albedo_surf);
        Renderer::Resources::TextureHandle normal_handle =
            resolveHandle(normal_path.empty() ? "__white_normal" : normal_path, normal_surf);

        std::string mr_key = std::format("__mr_{}_{}", metallic_path, roughness_path);
        Renderer::Resources::TextureHandle mr_handle{0};

        bool mr_found = false;
        for (const auto& entry : gpuHandleCache) {
            if (entry.path == mr_key) {
                mr_handle = entry.handle;
                mr_found = true;
                break;
            }
        }

        if (!mr_found) {
            SDL_Surface* mr_surf = BuildMR(metallic_surf, roughness_surf);
            mr_handle = mTextureManager.CreateTextureFromData(
                mr_surf->w, mr_surf->h, mr_surf->pixels, Renderer::TextureFormat::RGBA8, mr_surf->pitch,
                Renderer::TextureFilter::Linear, Renderer::TextureWrap::Repeat, mr_key, batch);
            SDL_DestroySurface(mr_surf);
            gpuHandleCache.push_back({mr_key, mr_handle});
        }

        if (normal_path.empty())
            SDL_DestroySurface(normal_surf);
        if (roughness_path.empty())
            SDL_DestroySurface(roughness_surf);
        if (metallic_path.empty())
            SDL_DestroySurface(metallic_surf);

        Renderer::Resources::MaterialHandle material = mMaterialManager.CreateMaterial(albedo_handle);
        mMaterialManager.SetNormalTexture(material, normal_handle);
        mMaterialManager.SetMetallicRoughnessTexture(material, mr_handle);

        float opacity = 1.0f;
        mat->Get(AI_MATKEY_OPACITY, opacity);

        bool transparent = opacity < 1.0f;

        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            SDL_Surface* surf = albedo_surf;

            if (surf) {
                const Uint8* pixels = static_cast<const Uint8*>(surf->pixels);

                bool hasAlpha = false;

                for (int y = 0; y < surf->h && !hasAlpha; ++y) {
                    for (int x = 0; x < surf->w; ++x) {
                        const Uint8* pixel = pixels + y * surf->pitch + x * 4;

                        if (pixel[3] < 255) {
                            hasAlpha = true;
                            break;
                        }
                    }
                }

                transparent |= hasAlpha;
            }
        }

        mMaterialManager.SetTransparent(material, transparent);

        float metallic_factor = 0.0f, roughness_factor = 1.0f;
        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic_factor);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor);
        mMaterialManager.SetMaterialMetallic(material, metallic_factor);
        mMaterialManager.SetMaterialRoughness(material, roughness_factor);
        if (albedo_path.empty())
            SDL_DestroySurface(albedo_surf);
        return material;
    }

    Renderer::Resources::Model ModelImporter::ImportModel(std::string path) {
        Utils::ScopedTimer total_timer("[3D Model Importer] Total import time");
        kModel model;
        CE_LOG(LogLevel::Debug, "[3D Model Importer] Attempting to load: {}", path);

        if (!mVFS.FileExists(path.c_str())) {
            CE_LOG(LogLevel::Error, "[3D Model Importer] File: {}, doesn't exist", path);
            return model;
        }

        Assimp::Importer importer;
        const aiScene* scene = nullptr;
        {
            Utils::ScopedTimer timer("[3D Model Importer] Assimp file read");
            importer.SetIOHandler(new CE::Assets::VFSIOSystem(&mVFS));
            scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace |
                                                aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
                                                aiProcess_SortByPType);
        }

        if (!scene || !scene->mRootNode) {
            CE_LOG(LogLevel::Error, "[3D Model Importer] Assimp error");
            return model;
        }

        CE_LOG(LogLevel::Debug, "RootNode pointer: {}", (void*)scene->mRootNode);
        CE_LOG(LogLevel::Debug, "Model: {}, mNumChildren: {}", path, scene->mRootNode->mNumChildren);
        for (unsigned int i = 0; i < std::min(scene->mRootNode->mNumChildren, 5u); ++i)
            CE_LOG(LogLevel::Debug, "Child {} Name: {}", i, scene->mRootNode->mChildren[i]->mName.C_Str());

        model.Materials.reserve(scene->mNumMaterials);
        model.Nodes.reserve(scene->mRootNode->mNumChildren * 8);
        model.MeshMaterialIndices.reserve(scene->mNumMeshes);

        std::vector<CE::Renderer::MeshData> mesh_data;
        mesh_data.reserve(scene->mNumMeshes);

        {
            Utils::ScopedTimer timer("[3D Model Importer] Mesh conversion");
            for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
                mesh_data.push_back(ConvertMesh(scene->mMeshes[i]));
                model.MeshMaterialIndices.push_back(scene->mMeshes[i]->mMaterialIndex);
            }
        }

        {
            Utils::ScopedTimer timer("[3D Model Importer] Material loading");

            std::unordered_map<std::string, SDL_Surface*> surfaceCache;
            std::vector<TextureInfo> gpuHandleCache;

            Renderer::TextureUploadBatch* batch = mRenderer.BeginBatchTextureUpload();

            for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
                model.Materials.push_back(
                    LoadAssimpMaterial(scene, scene->mMaterials[i], model, path, gpuHandleCache, surfaceCache, batch));
            }

            mRenderer.EndBatchTextureUpload(batch);

            for (auto& [key, surf] : surfaceCache) {
                SDL_DestroySurface(surf);
            }
        }

        {
            Utils::ScopedTimer timer("[3D Model Importer] GPU mesh upload");
            model.Meshes.reserve(scene->mNumMeshes);
            for (auto& mesh : mesh_data)
                model.Meshes.push_back(mGPUMeshManager.CreateMeshHandle(mesh));
        }

        model.RootNode = ConvertNode(scene->mRootNode, model);
        return model;
    }

} // namespace CE::Assets::Model3DImporter