#include "engine/rendering/common/material_manager.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    MaterialManager::MaterialManager(TextureManager& texture_manager, IRenderer& renderer) : mTextureManager(texture_manager), mRenderer(renderer) {}

    MaterialManager::MaterialEntry* MaterialManager::GetMaterialEntry(MaterialHandle handle) {
        auto list = mMaterials.find(handle);
        if (list != mMaterials.end()) {
            return &list->second;
        }
        return nullptr;
    }

    MaterialHandle MaterialManager::CreateMaterial(TextureHandle tex_handle) {
        MaterialEntry entry; 
        entry.TextureRef = mTextureManager.Acquire(tex_handle);

        if (!entry.TextureRef.IsValid()) {
            entry.IsError = true;
            CE::Log(LogLevel::Error, "[Material Manager] Invalid texture handle!");
            entry.Resource.albedo = mRenderer.GetErrorTexture();
        } else {
            entry.IsError = false;
            entry.Resource.albedo = entry.TextureRef.Get();
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


    void MaterialManager::SetMaterialMetallic(MaterialHandle matt_handle, float metallic) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->Resource.metallic = metallic;
        }
    }

    void MaterialManager::SetMaterialAlbedo(MaterialHandle matt_handle, TextureHandle tex_handle) {
        auto entry = GetMaterialEntry(matt_handle);
        if (entry) {
            entry->TextureRef = mTextureManager.Acquire(tex_handle);

            if (entry->TextureRef.IsValid()) {
                entry->Resource.albedo = entry->TextureRef.Get();
            } else {
                entry->Resource.albedo = mRenderer.GetErrorTexture();
            }
        }
    }

    void MaterialManager::DestroyMaterial(MaterialHandle handle) {
        auto it = mMaterials.find(handle);
        if (it == mMaterials.end())
            return;

        mMaterials.erase(it);
    }

    void MaterialManager::DestroyAllMaterials() {
        mMaterials.clear();
    }
}