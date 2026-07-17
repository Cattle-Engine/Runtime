#pragma once

#include "engine/common/containers/name_registry.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Common::Containers {
    struct RendererResourcesNameRegistry {
        NameRegistry<CE::Renderer::Resources::TextureHandle> Textures;
        NameRegistry<CE::Renderer::Resources::MaterialHandle> Materials;
        NameRegistry<CE::Renderer::Resources::MeshHandle> Meshs;
        NameRegistry<CE::Renderer::Resources::ShaderHandle> Shaders;
    };
} // namespace CE::Common::Containers