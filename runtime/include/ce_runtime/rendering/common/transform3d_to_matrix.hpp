#pragma once

#include "ce_runtime/rendering/renderer.hpp"

#include <glm/glm.hpp>

namespace CE::Renderer::Common {
    glm::mat4 Transform3DToMatrix(const Transform3D& t);
}