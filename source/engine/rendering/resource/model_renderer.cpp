#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/common/transform3d_to_matrix.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    ModelRenderer::ModelRenderer(
        MaterialManager& mat_manager, 
        GPUMeshManager& gpu_mesh_man, 
        IRenderer& renderer
    ) : mMaterialManager(mat_manager), mGPUMeshManager(gpu_mesh_man), mRenderer(renderer) {}

    void ModelRenderer::RenderModel(const Model& model, const Transform3D& transform)
    {
        if (model.Nodes.empty()) return;

        glm::mat4 rootMatrix = Common::Transform3DToMatrix(transform);

        RenderNode(model, model.RootNode, rootMatrix);
    }

    void ModelRenderer::RenderNode(
            const Model& model,
            uint32_t nodeIndex,
            const glm::mat4& parentTransform
        ) {
            if (nodeIndex >= model.Nodes.size()) return; 
            const Model::Node& node = model.Nodes[nodeIndex];

            glm::mat4 localTransform = node.Transform;

            glm::mat4 worldTransform = parentTransform * localTransform;

            for (uint32_t meshIndex : node.MeshIndices)
            {
                const MeshHandle meshHandle = model.Meshes[meshIndex];
                
                uint32_t materialIndex = model.MeshMaterialIndices[meshIndex];
                const MaterialHandle materialHandle = model.Materials[materialIndex];

                mGPUMeshManager.DrawMeshHandleMat4(
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

    void ModelRenderer::DestroyModel(Model& model) {
        for (auto meshHandle : model.Meshes) {
            mGPUMeshManager.DestroyMesh(meshHandle);
        }

        for (auto materialHandle : model.Materials) {
            mMaterialManager.DestroyMaterial(materialHandle);
        }

        model.Meshes.clear();
        model.Materials.clear();
        model.Nodes.clear();
        model.RootNode = 0;
    }
}