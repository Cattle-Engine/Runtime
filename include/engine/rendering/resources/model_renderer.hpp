#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"

namespace CE::Renderer::Resources {
    struct Model {
        struct Node {
            glm::mat4 Transform;
            std::vector<uint32_t> MeshIndices;
            std::vector<uint32_t> Children;
        };
        std::vector<MeshData> MeshesCPU;
        std::vector<Renderer::Resources::MeshHandle> Meshes;
        std::vector<Renderer::Resources::MaterialHandle> Materials;
        std::vector<Node> Nodes;

        uint32_t RootNode = 0;
    };

    class ModelRenderer {
        public: 
            ModelRenderer(
                MaterialManager& mat_manager, 
                GPUMeshManager& gpu_mesh_man, 
                IRenderer& renderer
            );
            void RenderModel(const Model& model, const Renderer::Transform3D& transform);
            void DestroyModel(Model& model);
        private:
            void RenderNode(
                const Model& model,
                uint32_t nodeIndex,
                const glm::mat4& parentTransform
            );

            MaterialManager& mMaterialManager;
            GPUMeshManager& mGPUMeshManager;
            IRenderer& mRenderer;
    };
} 