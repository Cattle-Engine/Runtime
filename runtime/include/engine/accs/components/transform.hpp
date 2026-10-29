#pragma once

#include "engine/accs/component.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::ACCS::Components {
    class Transform : public IComponent {
        public:
            Renderer::Transform3D transform_3d{};
            Renderer::Transform2D transform_2d{};
    };
}