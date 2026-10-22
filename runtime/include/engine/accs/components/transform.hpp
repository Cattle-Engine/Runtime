#pragma once

#include "engine/accs/component.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::ACCS::Components {
    class Transform : IComponent {
        Renderer::Transform3D transform_3d{};
        Renderer::Transform2D transfrom_2d{};
    };
}