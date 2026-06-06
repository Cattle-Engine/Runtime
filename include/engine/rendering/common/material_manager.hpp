#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/common/texture_manager.hpp"

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

            void SetMaterialAlbedo(MaterialHandle matt_handle, TextureHandle tex_handle);
            void SetMaterialColour(MaterialHandle matt_handle, Colour colour);
            void SetMaterialRoughness(MaterialHandle matt_handle, float roughness);
            void SetMaterialMetallic(MaterialHandle matt_handle, float metallic);
        private:
            struct MaterialEntry {
                Material Resource;
                TextureRef TextureRef;
                bool IsError;
            };

            MaterialEntry* GetMaterialEntry(MaterialHandle handle);
            uint64_t mNextHandleID = 0;
            TextureManager& mTextureManager;
            IRenderer& mRenderer;
            std::unordered_map<MaterialHandle, MaterialEntry> mMaterials;      
    };
}