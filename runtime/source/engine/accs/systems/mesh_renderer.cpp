#include "engine/accs/accs.hpp"
#include "engine/accs/components/3d_rendering.hpp"
#include "engine/accs/systems/mesh_renderer.hpp"
#include "engine/accs/components/transform.hpp"
#include "engine/accs/system.hpp"

namespace CE::ACCS::Systems {
    void MeshRenderer::Update(float dt, FrameUpdateType type, World& world, ACCS& accs) {
        if (type != FrameUpdateType::Render3D) return;

        world.Each<Components::Mesh, Components::Transform>(
            [&accs](const Components::Mesh& mesh, const Components::Transform& transform) {
                accs.dependencies.gpu_mesh_manager.DrawMeshHandle(
                    mesh.mesh, 
                    transform.transform_3d, 
                    mesh.material,
                    mesh.use_error_texture
                );
            }
        );
    }
}