#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Renderer::Resources {
    using MaterialHandle = uint64_t;
    class MaterialManager {
        public:
            MaterialManager(TextureManager& texture_manager, IRenderer& renderer);

            MaterialHandle CreateMaterial(TextureHandle tex_handle);
            
            // Internally used
            Material* GetMaterial(MaterialHandle matt_handle);
            void DestroyMaterial(MaterialHandle matt_handle);
            void DestroyAllMaterials();
            // Debug helpers
            size_t Debug_LoadedMaterialsCount() const;

            void SetMaterialAlbedo(MaterialHandle matt_handle, TextureHandle tex_handle);
            void SetMaterialColour(MaterialHandle matt_handle, Colour colour);
            void SetMaterialRoughness(MaterialHandle matt_handle, float roughness);
            void SetMaterialMetallic(MaterialHandle matt_handle, float metallic);

            void SetMetallicRoughnessTexture(MaterialHandle handle, TextureHandle tex_handle);
            void SetNormalTexture(MaterialHandle handle, TextureHandle tex_handle);
            void SetTransparent(MaterialHandle handle, bool transparent);
        private:
            struct MaterialEntry {
                Material Resource;
                TextureRef AlbedoTex;
                TextureRef MetallicRoughnessTex;
                TextureRef NormalTex;
                bool IsError;
            };

            MaterialEntry* GetMaterialEntry(MaterialHandle handle);
            uint64_t mNextHandleID = 0;
            TextureManager& mTextureManager;
            IRenderer& mRenderer;
            std::unordered_map<MaterialHandle, MaterialEntry> mMaterials;      
    };
}