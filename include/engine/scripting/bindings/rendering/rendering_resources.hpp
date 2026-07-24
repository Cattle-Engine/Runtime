#include <string>

#include "engine/rendering/resources/material_manager.hpp"
#include "engine/scripting/bindings/script_binding_class.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Scripting::Bindings {
    // Binds LoadTexture and UnloadTexture and adds the CE::Renderer::Resources::TextureHandle
    class TextureResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
        private:
            void LoadTexture(const std::string& path, Renderer::Resources::TextureHandle& out);
            void UnloadTexture(const Renderer::Resources::TextureHandle& handle);
    };
    
    // Binds the functions inside private and adds the CE::Renderer::Resources::MaterialHandle
    class MaterialResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
        private:
            void CreateMaterial(Renderer::Resources::MaterialHandle& handle, const Renderer::Resources::TextureHandle& tex);
            void DestroyMaterialHandle(const Renderer::Resources::MaterialHandle& handle);
            void SetMaterialAlbedo(
                const Renderer::Resources::MaterialHandle& handle, 
                const Renderer::Resources::TextureHandle tex
            );
            void SetMaterialTint(const Renderer::Resources::MaterialHandle& handle, const Renderer::Colour& colour);
            void SetMaterialRoughness(const Renderer::Resources::MaterialHandle& handle, float roughness);
            void SetMaterialMetallic(const Renderer::Resources::MaterialHandle& handle, float metallic);
            bool MaterialExists(const Renderer::Resources::MaterialHandle& handle) const;
    };
    
    class GPUMeshResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
    };
}