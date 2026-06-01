#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"

namespace CE::Assets::Textures {
    class TextureManager;
}

namespace CE::Renderer::Resources {
    class MaterialManager {
        public:
            MaterialManager(
                CE::Renderer::IRenderer* renderer,
                CE::Assets::Textures::TextureManager* textureManager = nullptr
            );

            void Load(const char* name);
            void Load(const char* name, const CE::Renderer::Material& material);
            void Unload(const char* name);
            void UnloadAll();

            CE::Renderer::Material* Get(const char* name);
            const CE::Renderer::Material* Get(const char* name) const;

            bool SetAlbedo(const char* name, const std::weak_ptr<CE::Renderer::Texture>& texture);
            bool SetAlbedo(const char* name, const char* textureName);
            bool SetAlbedo(
                const char* name,
                CE::Assets::Textures::TextureManager& textureManager,
                const char* textureName
            );

            bool SetTint(const char* name, CE::Renderer::Colour colour);
            bool SetRoughness(const char* name, float roughness);
            bool SetMetallic(const char* name, float metallic);

            bool Has(const char* name) const;
            int Debug_LoadedMaterialsCount() const;

        private:
            struct ManagedMaterial {
                CE::Renderer::Material Material {};
            };

            ManagedMaterial* FindMaterial(const char* name);
            const ManagedMaterial* FindMaterial(const char* name) const;

            [[maybe_unused]] CE::Renderer::IRenderer* mRenderer = nullptr;
            CE::Assets::Textures::TextureManager* mTextureManager = nullptr;
            std::unordered_map<std::string, ManagedMaterial> mMaterials;
    };
}
