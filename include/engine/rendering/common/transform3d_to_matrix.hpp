#pragma once

#include <glm/glm.hpp>

#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Common {
    glm::mat4 Transform3DToMatrix(const Transform3D& t);
}