#include "engine/scripting/angelscript.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/scripting/scripting_macros.hpp"

namespace CE::Scripting {
   bool Runtime::RegisterRegistryBindings() {
        CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER(
            mRendererResourcesNameRegistry.Materials,
            MaterialHandle,
            "CE::MaterialHandle",
            "CE::Registries::Materials",
            handle
        );

        CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER(
            mRendererResourcesNameRegistry.Meshs,
            MeshHandle,
            "CE::MeshHandle",
            "CE::Registries::Meshes",
            handle
        );

        CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER( 
            mRendererResourcesNameRegistry.Textures,
            TextureHandle,
            "CE::Texture",
            "CE::Registries::Textures",
            handle
        );

        CE_REGISTER_NAME_REGISTRY_GLOBAL(
            mRendererResourcesNameRegistry.Shaders,
            CE::Renderer::Resources::ShaderHandle,
            "CE::ShaderHandle",
            "CE::Registries::Shaders"
        );

        CE_LOG(CE::LogLevel::Info, "[AngelScript] Registry bindings registered successfully");
        return true;
    }
}