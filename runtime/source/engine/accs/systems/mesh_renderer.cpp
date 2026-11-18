#include "ce_runtime/accs/accs.hpp"
#include "ce_runtime/accs/components/3d_rendering.hpp"
#include "ce_runtime/accs/systems/mesh_renderer.hpp"
#include "ce_runtime/accs/components/transform.hpp"
#include "ce_runtime/accs/system.hpp"

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