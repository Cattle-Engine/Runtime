
#pragma once

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/accs/component.hpp"

#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"
#include "engine/accs/serialization_context.hpp"

namespace CE::ACCS::Components {
    class Mesh : public IComponent {
        public:
            Renderer::Resources::MeshHandle mesh;
            Renderer::Resources::MaterialHandle material;
            // If the material does not have an albedo, it will use the black/purple error texture when this is true
            bool use_error_texture = true;

            bool Serialize(CECommon::FS::BinaryWriter& writer, const SaveContext& ctx) const {
                if (!mesh) {
                    return false;
                }

                const auto mesh_data = ctx.GetDependencies().gpu_mesh_manager.ReadBackMeshData(mesh);

                if (!mesh_data.has_value()) {
                    return false;
                }

                const auto& data = mesh_data.value();

                if (data.vertices.size() > UINT32_MAX || data.indices.size() > UINT32_MAX) {
                    return false;
                }

                // Vertex count
                if (!writer.WriteU32(static_cast<uint32_t>(data.vertices.size()))) {
                    return false;
                }

                // Vertices
                for (const auto& vertex : data.vertices) {
                    // Position
                    if (!writer.WriteF32(vertex.position.x) ||
                        !writer.WriteF32(vertex.position.y) ||
                        !writer.WriteF32(vertex.position.z)) {
                        return false;
                    }

                    // Normal
                    if (!writer.WriteF32(vertex.normal.x) ||
                        !writer.WriteF32(vertex.normal.y) ||
                        !writer.WriteF32(vertex.normal.z)) {
                        return false;
                    }

                    // Colour
                    if (!writer.WriteF32(vertex.colour.r) ||
                        !writer.WriteF32(vertex.colour.g) ||
                        !writer.WriteF32(vertex.colour.b) ||
                        !writer.WriteF32(vertex.colour.a)) {
                        return false;
                    }

                    // UV
                    if (!writer.WriteF32(vertex.uv.x) ||
                        !writer.WriteF32(vertex.uv.y)) {
                        return false;
                    }

                    // Tangent
                    if (!writer.WriteF32(vertex.tangent.x) ||
                        !writer.WriteF32(vertex.tangent.y) ||
                        !writer.WriteF32(vertex.tangent.z)) {
                        return false;
                    }

                    // Tangent sign
                    if (!writer.WriteF32(vertex.tangentSign)) {
                        return false;
                    }
                }

                // Index count
                if (!writer.WriteU32(static_cast<uint32_t>(data.indices.size()))) {
                    return false;
                }

                // Indices
                for (const uint32_t index : data.indices) {
                    if (!writer.WriteU32(index)) {
                        return false;
                    }
                }

                // Material
                auto& material_manager = ctx.GetDependencies().material_manager;

                if (!material) {
                    return false;
                }

                const CE::Renderer::Material* mat = material_manager.GetMaterial(material);

                if (!mat) {
                    return false;
                }

                if (!writer.WriteF32(mat->tint.r) ||
                    !writer.WriteF32(mat->tint.g) ||
                    !writer.WriteF32(mat->tint.b) ||
                    !writer.WriteF32(mat->tint.a)) {
                    return false;
                }

                if (!writer.WriteF32(mat->roughness) ||
                    !writer.WriteF32(mat->metallic)) {
                    return false;
                }

                if (!writer.WriteF32(mat->emissionColour.r) ||
                    !writer.WriteF32(mat->emissionColour.g) ||
                    !writer.WriteF32(mat->emissionColour.b) ||
                    !writer.WriteF32(mat->emissionColour.a)) {
                    return false;
                }

                if (!writer.WriteF32(mat->emissionStrength)) {
                    return false;
                }

                if (!writer.WriteU8(mat->isTransparent ? 1 : 0) ||
                    !writer.WriteU8(mat->isDoubleSided ? 1 : 0) ||
                    !writer.WriteBool(use_error_texture)) {
                    return false;
                }

                // Texture paths
                auto& texture_manager = ctx.GetDependencies().texture_manager;

                return writer.WriteSizedString(texture_manager.GetTexturePath(
                        Renderer::Resources::TextureHandle(material_manager.GetAlbedoTextureID(material)))) &&
                    writer.WriteSizedString(texture_manager.GetTexturePath(
                        Renderer::Resources::TextureHandle(material_manager.GetMetallicRoughnessTextureID(material)))) &&
                    writer.WriteSizedString(texture_manager.GetTexturePath(
                        Renderer::Resources::TextureHandle(material_manager.GetNormalTextureID(material)))) &&
                    writer.WriteSizedString(texture_manager.GetTexturePath(
                        Renderer::Resources::TextureHandle(material_manager.GetEmissionTextureID(material))));
            }

            bool Deserialize(CECommon::FS::BinaryReader& reader, const LoadContext& ctx) {
                using namespace CE::Renderer::Resources;

                CE::Renderer::MeshData data;

                // Vertex count
                uint32_t vertex_count;
                if (!reader.ReadU32(vertex_count)) {
                    return false;
                }

                data.vertices.resize(vertex_count);

                // Vertices
                for (auto& vertex : data.vertices) {
                    if (!reader.ReadF32(vertex.position.x) ||
                        !reader.ReadF32(vertex.position.y) ||
                        !reader.ReadF32(vertex.position.z)) {
                        return false;
                    }

                    if (!reader.ReadF32(vertex.normal.x) ||
                        !reader.ReadF32(vertex.normal.y) ||
                        !reader.ReadF32(vertex.normal.z)) {
                        return false;
                    }

                    if (!reader.ReadF32(vertex.colour.r) ||
                        !reader.ReadF32(vertex.colour.g) ||
                        !reader.ReadF32(vertex.colour.b) ||
                        !reader.ReadF32(vertex.colour.a)) {
                        return false;
                    }

                    if (!reader.ReadF32(vertex.uv.x) ||
                        !reader.ReadF32(vertex.uv.y)) {
                        return false;
                    }

                    if (!reader.ReadF32(vertex.tangent.x) ||
                        !reader.ReadF32(vertex.tangent.y) ||
                        !reader.ReadF32(vertex.tangent.z)) {
                        return false;
                    }

                    if (!reader.ReadF32(vertex.tangentSign)) {
                        return false;
                    }
                }

                // Index count
                uint32_t index_count;
                if (!reader.ReadU32(index_count)) {
                    return false;
                }

                data.indices.resize(index_count);

                // Indices
                for (auto& index : data.indices) {
                    if (!reader.ReadU32(index)) {
                        return false;
                    }
                }

                mesh = ctx.GetDependencies().gpu_mesh_manager.CreateMeshHandle(data);

                // Material
                CE::Renderer::Colour tint;
                float roughness;
                float metallic;
                CE::Renderer::Colour emission_colour;
                float emission_strength;
                uint8_t is_transparent;
                uint8_t is_double_sided;
                bool loaded_use_error_texture;

                if (!reader.ReadF32(tint.r) ||
                    !reader.ReadF32(tint.g) ||
                    !reader.ReadF32(tint.b) ||
                    !reader.ReadF32(tint.a)) {
                    return false;
                }

                if (!reader.ReadF32(roughness) ||
                    !reader.ReadF32(metallic)) {
                    return false;
                }

                if (!reader.ReadF32(emission_colour.r) ||
                    !reader.ReadF32(emission_colour.g) ||
                    !reader.ReadF32(emission_colour.b) ||
                    !reader.ReadF32(emission_colour.a)) {
                    return false;
                }

                if (!reader.ReadF32(emission_strength)) {
                    return false;
                }

                if (!reader.ReadU8(is_transparent) ||
                    !reader.ReadU8(is_double_sided) ||
                    !reader.ReadBool(loaded_use_error_texture)) {
                    return false;
                }

                // Read texture paths
                std::string albedo_path;
                std::string metallic_roughness_path;
                std::string normal_path;
                std::string emission_path;

                if (!reader.ReadSizedString(albedo_path) ||
                    !reader.ReadSizedString(metallic_roughness_path) ||
                    !reader.ReadSizedString(normal_path) ||
                    !reader.ReadSizedString(emission_path)) {
                    return false;
                }

                auto& texture_manager = ctx.GetDependencies().texture_manager;
                auto& material_manager = ctx.GetDependencies().material_manager;

                // Reconstruct material
                material = material_manager.CreateMaterial();

                material_manager.SetMaterialColour(material, tint);
                material_manager.SetMaterialRoughness(material, roughness);
                material_manager.SetMaterialMetallic(material, metallic);

                material_manager.SetEmissionColour(material, emission_colour);
                material_manager.SetEmissionStrength(material, emission_strength);

                material_manager.SetTransparent(material, is_transparent != 0);
                material_manager.SetDoubleSided(material, is_double_sided != 0);

                use_error_texture = loaded_use_error_texture;

                // Restore textures from paths
                if (!albedo_path.empty()) {
                    material_manager.SetMaterialAlbedo(material, texture_manager.Load(albedo_path));
                }

                if (!metallic_roughness_path.empty()) {
                    material_manager.SetMetallicRoughnessTexture(material, texture_manager.Load(metallic_roughness_path));
                }

                if (!normal_path.empty()) {
                    material_manager.SetNormalTexture(material, texture_manager.Load(normal_path));
                }

                if (!emission_path.empty()) {
                    material_manager.SetEmissionTexture(material, texture_manager.Load(emission_path));
                }

                return true;
            }
    };
}
