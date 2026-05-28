#include "engine/rendering/common/material_manager.hpp"

#include "engine/assets/textures.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    MaterialManager::MaterialManager(
        CE::Renderer::IRenderer* renderer,
        CE::Assets::Textures::TextureManager* textureManager
    ) :
        mRenderer(renderer),
        mTextureManager(textureManager) {
    }

    MaterialManager::ManagedMaterial* MaterialManager::FindMaterial(const char* name) {
        auto it = mMaterials.find(name ? name : "");
        return it != mMaterials.end() ? &it->second : nullptr;
    }

    const MaterialManager::ManagedMaterial* MaterialManager::FindMaterial(const char* name) const {
        auto it = mMaterials.find(name ? name : "");
        return it != mMaterials.end() ? &it->second : nullptr;
    }

    void MaterialManager::Load(const char* name) {
        if (!name || name[0] == '\0') {
            CE::Log(LogLevel::Error, "[Material Manager] Load called with an empty name");
            return;
        }

        mMaterials[name] = ManagedMaterial{};
    }

    void MaterialManager::Load(const char* name, const CE::Renderer::Material& material) {
        if (!name || name[0] == '\0') {
            CE::Log(LogLevel::Error, "[Material Manager] Load called with an empty name");
            return;
        }

        mMaterials[name] = ManagedMaterial{material};
    }

    void MaterialManager::Unload(const char* name) {
        auto it = mMaterials.find(name ? name : "");
        if (it == mMaterials.end()) {
            return;
        }

        mMaterials.erase(it);
    }

    void MaterialManager::UnloadAll() {
        mMaterials.clear();
    }

    CE::Renderer::Material* MaterialManager::Get(const char* name) {
        ManagedMaterial* material = FindMaterial(name);
        return material ? &material->Material : nullptr;
    }

    const CE::Renderer::Material* MaterialManager::Get(const char* name) const {
        const ManagedMaterial* material = FindMaterial(name);
        return material ? &material->Material : nullptr;
    }

    bool MaterialManager::SetAlbedo(const char* name, CE::Renderer::Texture* texture) {
        ManagedMaterial* material = FindMaterial(name);
        if (!material) {
            CE::Log(LogLevel::Warn, "[Material Manager] Tried to set albedo on missing material '{}'", name ? name : "");
            return false;
        }

        material->Material.albedo = texture;
        return true;
    }

    bool MaterialManager::SetAlbedo(const char* name, const char* textureName) {
        if (!mTextureManager) {
            CE::Log(LogLevel::Error, "[Material Manager] No texture manager available for SetAlbedo");
            return false;
        }

        return SetAlbedo(name, *mTextureManager, textureName);
    }

    bool MaterialManager::SetAlbedo(
        const char* name,
        CE::Assets::Textures::TextureManager& textureManager,
        const char* textureName
    ) {
        ManagedMaterial* material = FindMaterial(name);
        if (!material) {
            CE::Log(LogLevel::Warn, "[Material Manager] Tried to set albedo on missing material '{}'", name ? name : "");
            return false;
        }

        CE::Renderer::Texture* texture = textureManager.Get(textureName);
        if (!texture) {
            CE::Log(LogLevel::Error, "[Material Manager] Texture '{}' was not found for material '{}'", textureName ? textureName : "", name ? name : "");
            if (mRenderer) {
                material->Material.albedo = mRenderer->GetErrorTexture();
            }
            return false;
        }

        material->Material.albedo = texture;
        return true;
    }

    bool MaterialManager::SetTint(const char* name, CE::Renderer::Colour colour) {
        ManagedMaterial* material = FindMaterial(name);
        if (!material) {
            CE::Log(LogLevel::Warn, "[Material Manager] Tried to set tint on missing material '{}'", name ? name : "");
            return false;
        }

        material->Material.tint = colour;
        return true;
    }

    bool MaterialManager::SetRoughness(const char* name, float roughness) {
        ManagedMaterial* material = FindMaterial(name);
        if (!material) {
            CE::Log(LogLevel::Warn, "[Material Manager] Tried to set roughness on missing material '{}'", name ? name : "");
            return false;
        }

        material->Material.roughness = roughness;
        return true;
    }

    bool MaterialManager::SetMetallic(const char* name, float metallic) {
        ManagedMaterial* material = FindMaterial(name);
        if (!material) {
            CE::Log(LogLevel::Warn, "[Material Manager] Tried to set metallic on missing material '{}'", name ? name : "");
            return false;
        }

        material->Material.metallic = metallic;
        return true;
    }

    bool MaterialManager::Has(const char* name) const {
        return mMaterials.find(name ? name : "") != mMaterials.end();
    }

    int MaterialManager::Debug_LoadedMaterialsCount() const {
        return static_cast<int>(mMaterials.size());
    }
}
