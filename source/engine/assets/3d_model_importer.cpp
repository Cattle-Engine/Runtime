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

        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& f = mesh->mFaces[i];

            for (uint32_t j = 0; j < f.mNumIndices; j++)
                out.indices.push_back(f.mIndices[j]);
        }

        out.vertex_count = (uint32_t)out.vertices.size();
        out.indice_count = (uint32_t)out.indices.size();

        // Generate tangents and handedness (tangentSign) for normal mapping.
        // Prefer Assimp-provided tangents/bitangents if available, otherwise compute from positions+UVs.
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
                // Orthonormalize tangent with respect to normal
                tangent = glm::normalize(tangent - n * glm::dot(n, tangent));

                float handedness = (glm::dot(glm::cross(n, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

                out.vertices[i].tangent = tangent;
                out.vertices[i].tangentSign = handedness;
            }
        } else if (mesh->HasTextureCoords(0)) {
            // Compute tangents from triangle data
            std::vector<glm::vec3> tan1(out.vertices.size(), glm::vec3(0.0f));
            std::vector<glm::vec3> tan2(out.vertices.size(), glm::vec3(0.0f));

            for (size_t f = 0; f + 2 < out.indices.size(); f += 3) {
                uint32_t i1 = out.indices[f + 0];
                uint32_t i2 = out.indices[f + 1];
                uint32_t i3 = out.indices[f + 2];

                const glm::vec3& v1 = out.vertices[i1].position;
                const glm::vec3& v2 = out.vertices[i2].position;
                const glm::vec3& v3 = out.vertices[i3].position;

                const glm::vec2& w1 = out.vertices[i1].uv;
                const glm::vec2& w2 = out.vertices[i2].uv;
                const glm::vec2& w3 = out.vertices[i3].uv;

                float x1 = v2.x - v1.x;
                float x2 = v3.x - v1.x;
                float y1 = v2.y - v1.y;
                float y2 = v3.y - v1.y;
                float z1 = v2.z - v1.z;
                float z2 = v3.z - v1.z;

                float s1 = w2.x - w1.x;
                float s2 = w3.x - w1.x;
                float t1 = w2.y - w1.y;
                float t2 = w3.y - w1.y;

                float denom = (s1 * t2 - s2 * t1);
                float r = denom == 0.0f ? 0.0f : 1.0f / denom;

                glm::vec3 sdir = glm::vec3((t2 * x1 - t1 * x2) * r,
                                           (t2 * y1 - t1 * y2) * r,
                                           (t2 * z1 - t1 * z2) * r);
                glm::vec3 tdir = glm::vec3((s1 * x2 - s2 * x1) * r,
                                           (s1 * y2 - s2 * y1) * r,
                                           (s1 * z2 - s2 * z1) * r);

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

                // Gram-Schmidt orthogonalize
                glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

                // Calculate handedness
                float handedness = (glm::dot(glm::cross(n, tangent), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

                if (!(std::isfinite(tangent.x) && std::isfinite(tangent.y) && std::isfinite(tangent.z)) || glm::dot(tangent, tangent) < 1e-8f) {
                    tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                    handedness = 1.0f;
                }

                out.vertices[i].tangent = tangent;
                out.vertices[i].tangentSign = handedness;
            }
        } else {
            // No UVs/tangents: provide a default tangent
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

        SDL_Surface* out = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, 0, 32);

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

    SDL_Surface* ModelImporter::LoadAssimpTexture(
        const aiScene* scene,
        const aiMaterial* mat,
        aiTextureType type,
        Renderer::Resources::Model& model,
        std::string mdl_path
    ) {
        aiString tex_path;

        if (mat->GetTexture(type, 0, &tex_path) == AI_SUCCESS) {
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
                        if (!rw) return CreateWhiteSurface();

                        SDL_Surface* surface = IMG_Load_IO(rw, 1);
                        SDL_Surface* formated_texture;

                        if (!surface) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to create SDL_Surface");
                            SDL_DestroySurface(surface);
                            return CreateWhiteSurface();
                        }

                        formated_texture = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);

                        if (!formated_texture) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
                            SDL_DestroySurface(surface);
                            SDL_DestroySurface(formated_texture);
                            return CreateWhiteSurface();
                        }

                        SDL_DestroySurface(surface);

                        return formated_texture;
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
                            return CreateWhiteSurface();
                        }

                        formated_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);

                        if (!formated_surface) {
                            CE::Log(LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
                            SDL_DestroySurface(surface);
                            SDL_DestroySurface(formated_surface);
                            return CreateWhiteSurface();
                        }

                        SDL_DestroySurface(surface);

                        return formated_surface;
                    }
                } catch (const std::invalid_argument&) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] std::stoi threw invalid argument at position: {}", path_string_pos);
                    return CreateWhiteSurface();
                } catch (const std::out_of_range&) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] std::stoi throw out of range");
                    return CreateWhiteSurface();
                }
            } else {
                std::filesystem::path base(mdl_path);
                std::filesystem::path combined = base / path;

                std::string virtual_path = combined.generic_string();
                virtual_path = mVFS.NormalizeVirtualPath(virtual_path);

                if (!mVFS.FileExists(virtual_path.c_str())) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] Texture doesn't exist: {}", virtual_path);
                    return CreateWhiteSurface();
                }

                VirtualFile* file = mVFS.OpenFile(virtual_path.c_str());

                SDL_Surface* surface = IMG_Load_IO(file->sdl_stream, false);

                if (!surface) {
                    CE::Log(LogLevel::Error, "[3D Model Importer] Failed to load texture: {}, Error is: {}", virtual_path, SDL_GetError());
                }

                if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                    SDL_Surface* formated_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
                    if (!formated_surface) {
                        CE::Log(LogLevel::Error, "[3D Model Importer] Failed to convert surface to RGBA32");
                        return CreateWhiteSurface();
                    } else {
                        return formated_surface;
                    }
                } else {
                    return surface;
                }
            }
        }
        return CreateWhiteSurface();
    }


    Renderer::Resources::MaterialHandle ModelImporter::LoadAssimpMaterial(
        const aiScene* scene,
        const aiMaterial* mat,
        Renderer::Resources::Model& model,
        std::string model_path
    ) {
        SDL_Surface* albedo = LoadAssimpTexture(scene, mat, aiTextureType_DIFFUSE, model, model_path);
        SDL_Surface* normal = LoadAssimpTexture(scene, mat, aiTextureType_NORMALS, model, model_path);
        SDL_Surface* roughness_tex = LoadAssimpTexture(scene, mat, aiTextureType_DIFFUSE_ROUGHNESS, model, model_path);
        SDL_Surface* metallic_tex = LoadAssimpTexture(scene, mat, aiTextureType_METALNESS, model, model_path);

        auto albedo_handle = mTextureManager.CreateTextureFromData(
            albedo->w,
            albedo->h,
            albedo->pixels,
            Renderer::TextureFormat::RGBA8,
            albedo->pitch
        );

        auto normal_handle = mTextureManager.CreateTextureFromData(
            normal->w,
            normal->h,
            normal->pixels,
            Renderer::TextureFormat::RGBA8,
            normal->pitch
        );
                        /* No idea if this is just my ide but for some reason BuildMR had too many paramters, doing this-> fixed it */
        SDL_Surface* mr_texture = this->BuildMR(metallic_tex, roughness_tex);

        auto mr_handle = mTextureManager.CreateTextureFromData(
            mr_texture->w,
            mr_texture->h,
            mr_texture->pixels,
            Renderer::TextureFormat::RGBA8,
            mr_texture->pitch
        );

        SDL_DestroySurface(albedo);
        SDL_DestroySurface(normal);
        SDL_DestroySurface(roughness_tex);
        SDL_DestroySurface(metallic_tex);

        Renderer::Resources::MaterialHandle material = mMaterialManager.CreateMaterial(albedo_handle);
        mMaterialManager.SetNormalTexture(material, normal_handle);
        mMaterialManager.SetMetallicRoughnessTexture(material, mr_handle);

        float metallic;
        float roughness;

        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    
        mMaterialManager.SetMaterialMetallic(material, metallic);
        mMaterialManager.SetMaterialRoughness(material, roughness);
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