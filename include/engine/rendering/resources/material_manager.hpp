#pragma once

#include <cstdint>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Renderer::Resources {
    struct MaterialHandle {
        MaterialHandle() : id(0) {}
        MaterialHandle(const MaterialHandle& other) : id(other.id) {}
        
        uint64_t id = 0;
        
        explicit operator bool() const {
          return id != 0;
        }
        
        bool operator==(const MaterialHandle& other) const {
          return id == other.id;
        }
    };
    
    struct MaterialHandleHash {
        size_t operator()(const MaterialHandle& handle) const {
          return std::hash<uint64_t>{}(handle.id);
        }
    };
    
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
        std::unordered_map<MaterialHandle, MaterialEntry, MaterialHandleHash> mMaterials;
    };
} // namespace CE::Renderer::Resources