#include "engine/rendering/resources/material_manager.hpp"

#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"

namespace CE::Renderer::Resources {
    MaterialManager::MaterialManager(TextureManager& texture_manager, IRenderer& renderer, ShaderManager& shader_manager)
        : mTextureManager(texture_manager), mShaderManager(shader_manager), mRenderer(renderer) {}

    MaterialManager::MaterialEntry* MaterialManager::GetMaterialEntry(MaterialHandle handle) {
        auto list = mMaterials.find(handle);
        if (list != mMaterials.end()) {
            return &list->second;
        }
        return nullptr;
    }

    MaterialHandle MaterialManager::CreateMaterial(TextureHandle tex_handle) {
        MaterialEntry entry;
        entry.AlbedoTex = mTextureManager.Acquire(tex_handle);

        if (!entry.AlbedoTex.IsValid()) {
            entry.IsError = true;
            CE_LOG(LogLevel::Error, "[Material Manager] Invalid texture handle!");
            entry.Resource.albedo = mRenderer.GetErrorTexture();
        } else {
            entry.IsError = false;
            entry.Resource.albedo = entry.AlbedoTex.Get();
        }
        MaterialHandle handle;
        handle.id = mNextHandleID++;
        mMaterials.emplace(handle, std::move(entry));
        return handle;
    }

    MaterialHandle MaterialManager::CreateMaterial() {
        MaterialEntry entry;
        entry.IsError = false;
        MaterialHandle handle;
        handle.id = mNextHandleID++;
        mMaterials.emplace(handle, std::move(entry));
        return handle;     
    }

    void MaterialManager::SetMaterialColour(MaterialHandle matt_handle, Colour colour) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->Resource.tint = colour;
        }
    }

    void MaterialManager::SetMaterialRoughness(MaterialHandle matt_handle, float roughness) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->Resource.roughness = roughness;
        }
    }

    void MaterialManager::SetTransparent(MaterialHandle handle, bool transparent) {
        auto entry = GetMaterialEntry(handle);
        if (entry) {
            entry->Resource.isTransparent = transparent;
        }
    }

    void MaterialManager::SetDoubleSided(MaterialHandle handle, bool doubleSided) {
        auto entry = GetMaterialEntry(handle);
        if (entry) {
            entry->Resource.isDoubleSided = doubleSided;
        }
    }

    void MaterialManager::SetMaterialMetallic(MaterialHandle matt_handle, float metallic) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->Resource.metallic = metallic;
        }
    }

    void MaterialManager::SetMaterialAlbedo(MaterialHandle matt_handle, TextureHandle tex_handle) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->AlbedoTex = mTextureManager.Acquire(tex_handle);

            if (entry->AlbedoTex.IsValid()) {
                entry->Resource.albedo = entry->AlbedoTex.Get();
            } else {
                entry->Resource.albedo = mRenderer.GetErrorTexture();
            }
        }
    }

    void MaterialManager::SetMetallicRoughnessTexture(MaterialHandle matt_handle, TextureHandle tex_handle) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->MetallicRoughnessTex = mTextureManager.Acquire(tex_handle);

            if (entry->MetallicRoughnessTex.IsValid()) {
                entry->Resource.metallicRoughnessTex = entry->MetallicRoughnessTex.Get();
            } else {
                entry->Resource.metallicRoughnessTex = nullptr;
            }
        }
    }

    void MaterialManager::SetNormalTexture(MaterialHandle matt_handle, TextureHandle tex_handle) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->NormalTex = mTextureManager.Acquire(tex_handle);

            if (entry->NormalTex.IsValid()) {
                entry->Resource.normal = entry->NormalTex.Get();
            } else {
                entry->Resource.normal = nullptr;
            }
        }
    }

    void MaterialManager::DestroyMaterial(MaterialHandle handle) {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end())
            return;

        mMaterials.erase(it);
    }

    Material* MaterialManager::GetMaterial(MaterialHandle handle) {
        auto material = GetMaterialEntry(handle);

        if (material) {
            return &material->Resource;
        } else {
            return nullptr;
        }
    }

    void MaterialManager::DestroyAllMaterials() {
        mMaterials.clear();
    }

    size_t MaterialManager::Debug_LoadedMaterialsCount() const {
        return mMaterials.size();
    }

    void MaterialManager::SetShader(MaterialHandle handle,ShaderHandle shader) {
        auto* material = GetMaterialEntry(handle);

        if (material) {
            material->Shader = mShaderManager.AcquireRef(shader);
            if (!material->Shader.IsValid()) {
                material->Resource.shader = nullptr;
            } else {
                material->Resource.shader = material->Shader.Get();
            }
        }
    }

    bool MaterialManager::SetEmissionColour(MaterialHandle handle, Colour colour) {
        auto material = GetMaterial(handle);

        if (material) {
            material->emissionColour = colour;
            return true;
        }

        CE_LOG(LogLevel::Error, "[Material Manager] [SetEmissionColour] Material {} not found.", handle.id);
        return false;
    }

    bool MaterialManager::SetEmissionStrength(MaterialHandle handle, float strength) {
        auto material = GetMaterial(handle);

        if (material) {
            material->emissionStrength = strength;
            return true;
        }
        CE_LOG(LogLevel::Error, "[Material Manager] [SetEmissionStrength] Material {} not found.", handle.id);
        return false;
    }

    bool MaterialManager::SetEmissionTexture(MaterialHandle handle, TextureHandle tex_handle) {
        auto material = GetMaterialEntry(handle);

        if (material) {
            material->EmissionTex = mTextureManager.Acquire(tex_handle);
            if (!material->EmissionTex.IsValid()) {
                CE_LOG(LogLevel::Error, "[Material Manager] [SetEmissionTexture] Could not find texture handle");
                material->EmissionTex.Reset();
                material->Resource.emissionTex = nullptr;
                return false;
            }
            return true;
        }
        return false;
    }

    bool MaterialManager::GetMaterialColour(MaterialHandle handle, Colour& outColour) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outColour = it->second.Resource.tint;
        return true;
    }

    bool MaterialManager::GetMaterialRoughness(MaterialHandle handle, float& outRoughness) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outRoughness = it->second.Resource.roughness;
        return true;
    }

    bool MaterialManager::GetMaterialMetallic(MaterialHandle handle, float& outMetallic) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outMetallic = it->second.Resource.metallic;
        return true;
    }

    uint64_t MaterialManager::GetAlbedoTextureID(MaterialHandle handle) const {
        auto it = mMaterials.find(handle);
        return it != mMaterials.end() ? it->second.AlbedoTex.GetHandleID() : 0;
    }

    uint64_t MaterialManager::GetMetallicRoughnessTextureID(MaterialHandle handle) const {
        auto it = mMaterials.find(handle);
        return it != mMaterials.end() ? it->second.MetallicRoughnessTex.GetHandleID() : 0;
    }

    uint64_t MaterialManager::GetNormalTextureID(MaterialHandle handle) const {
        auto it = mMaterials.find(handle);
        return it != mMaterials.end() ? it->second.NormalTex.GetHandleID() : 0;
    }

    uint64_t MaterialManager::GetEmissionTextureID(MaterialHandle handle) const {
        auto it = mMaterials.find(handle);
        return it != mMaterials.end() ? it->second.EmissionTex.GetHandleID() : 0;
    }
    
    bool MaterialManager::GetEmissionColour(MaterialHandle handle, Colour& outColour) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outColour = it->second.Resource.emissionColour;
        return true;
    }

    bool MaterialManager::GetEmissionStrength(MaterialHandle handle, float& outStrength) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outStrength = it->second.Resource.emissionStrength;
        return true;
    }

    bool MaterialManager::GetTransparent(MaterialHandle handle, bool& outTransparent) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outTransparent = it->second.Resource.isTransparent;
        return true;
    }

    bool MaterialManager::GetDoubleSided(MaterialHandle handle, bool& outDoubleSided) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outDoubleSided = it->second.Resource.isDoubleSided;
        return true;
    }

    bool MaterialManager::GetShader(MaterialHandle handle, Shader*& outShader) const {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end()) return false;

        outShader = it->second.Resource.shader;
        return true;
    }
} // namespace CE::Renderer::Resources
