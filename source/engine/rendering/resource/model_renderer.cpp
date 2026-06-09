#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/common/transform3d_to_matrix.hpp"

namespace CE::Renderer::Resources {
    ModelRenderer::ModelRenderer(
        MaterialManager& mat_manager, 
        GPUMeshManager& gpu_mesh_man, 
        IRenderer& renderer
    ) : mMaterialManager(mat_manager), mGPUMeshManager(gpu_mesh_man), mRenderer(renderer) {}

    void ModelRenderer::RenderModel(const Model& model, const Transform3D& transform) {
        if (model.Nodes.empty()) return;

        RenderNode(model, model.RootNode, transform);
    }

    void ModelRenderer::RenderNode(
        const Model& model,
        uint32_t nodeIndex,
        const glm::mat4& parentTransform
    ) {
        const Model::Node& node = model.Nodes[nodeIndex];

        glm::mat4 localTransform = Common::Transform3DToMatrix(node.Transform);
        glm::mat4 worldTransform = parentTransform * localTransform;

        for (uint32_t meshIndex : node.MeshIndices)
        {
            const MeshHandle meshHandle = model.Meshes[meshIndex];
            const MaterialHandle materialHandle = model.Materials[meshIndex];

            mGPUMeshManager.DrawMeshHandle(
                meshHandle,
                worldTransform,
                materialHandle,
                false
            );
        }

        for (uint32_t childIndex : node.Children) {
            RenderNode(model, childIndex, worldTransform);
        }
    }
}