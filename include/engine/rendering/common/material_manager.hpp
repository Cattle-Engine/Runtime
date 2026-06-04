#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/common/texture_manager.hpp"

namespace CE::Renderer::Resources {
    using MaterialHandle = uint64_t;
    class MaterialManager {
        public:
            MaterialManager(TextureManager& texture_manager);
            MaterialHandle CreateMaterial(TextureHandle tex_handle);
            void SetMaterialAlbedo(MaterialHandle matt_handle, TextureHandle tex_handle);
            void SetMaterialColour(MaterialHandle matt_handle, Colour colour);
            void SetMaterialRoughness(MaterialHandle matt_handle, float roughness);
            void SetMaterialMetallic(MaterialHandle matt_handle, float metallic);
        private:
            std::unordered_map<MaterialHandle, Material> mMaterials;
    };
}