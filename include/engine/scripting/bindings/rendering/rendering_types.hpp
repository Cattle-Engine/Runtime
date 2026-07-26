#pragma once

#include "engine/scripting/bindings/script_binding_class.hpp"

namespace CE::Scripting::Bindings {
    struct ASMeshData {

    };

    // Stuff that I don't know where to give a home
    class RenderingTypes : public IScriptBinding {
        public:
            bool RegisterBindings() override;
        private:
            // Registers CE::Renderer::Colour
            bool RegisterColourBinding();
            // Registers CE::Renderer::Resources::TextureHandle/MeshHandle/MaterialHandle to CE:: in angelscript
            bool RegisterResourceHandleBindings();
            // Binds glm::vec3 and glm::vec2 to CE::Vec3, CE::Vec2
            bool RegisterVec3AndVec2();
            bool RegisterVertex3D();
    };
}