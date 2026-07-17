#include "engine/rendering/common/transform3d_to_matrix.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace CE::Renderer::Common {
    glm::mat4 Transform3DToMatrix(const Transform3D &t) {
        glm::mat4 transform(1.0f);

        transform = glm::translate(transform, t.position);

        transform = glm::rotate(transform, t.rotation.x, glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, t.rotation.y, glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, t.rotation.z, glm::vec3(0, 0, 1));

        transform = glm::scale(transform, t.scale);

        return transform;
    }
} // namespace CE::Renderer::Common