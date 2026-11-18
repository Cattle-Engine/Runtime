#pragma once

#include "ce_runtime/common/containers/name_registry.hpp"
#include "ce_runtime/rendering/resources/gpu_mesh_manager.hpp"
#include "ce_runtime/rendering/resources/material_manager.hpp"
#include "ce_runtime/rendering/resources/shader_manager.hpp"
#include "ce_runtime/rendering/resources/texture_manager.hpp"

namespace CE::Common::Containers {
    struct RendererResourcesNameRegistry {
        NameRegistry<CE::Renderer::Resources::TextureHandle> Textures;
        NameRegistry<CE::Renderer::Resources::MaterialHandle> Materials;
        NameRegistry<CE::Renderer::Resources::MeshHandle> Meshs;
        NameRegistry<CE::Renderer::Resources::ShaderHandle> Shaders;
    };
} // namespace CE::Common::Containers