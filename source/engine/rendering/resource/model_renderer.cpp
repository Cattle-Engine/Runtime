#include "engine/rendering/resources/model_renderer.hpp"

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
        const Transform3D& parentTransform
    ) { 
        Transform3D parent_transform = parentTransform;
        const Model::Node& node = model.Nodes[nodeIndex];
        Transform3D worldTransform = parent_transform * node.Transform; // Ik I need to fix... 1am coding strikes
        for (uint32_t meshIndex : node.MeshIndices) {
            const MeshHandle meshHandle = model.Meshes[meshIndex];

            const MaterialHandle materialHandle = model.Materials[meshIndex];

            Material* material = mMaterialManager.GetMaterial(materialHandle);

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