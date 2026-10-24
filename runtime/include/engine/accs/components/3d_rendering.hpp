#pragma once

#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/accs/component.hpp"

namespace CE::ACCS::Components {
    class Mesh : public IComponent {
        public:
            Renderer::Resources::MeshHandle mesh;
            Renderer::Resources::MaterialHandle material;
            // If the material does not have an albedo, it will use the black/purple error texture when this is true
            bool use_error_texture = true;
    };
}