#include "engine/rendering/resources/material_manager.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    MaterialManager::MaterialManager(TextureManager &texture_manager, IRenderer &renderer)
        : mTextureManager(texture_manager), mRenderer(renderer) {}

    MaterialManager::MaterialEntry *MaterialManager::GetMaterialEntry(MaterialHandle handle) {
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

        MaterialHandle handle = mNextHandleID++;
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

    Material *MaterialManager::GetMaterial(MaterialHandle handle) {
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
} // namespace CE::Renderer::Resources