#include <memory>

#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_RendererResourceManagers() {
        gTextureManager = std::make_unique<CE::Renderer::Resources::TextureManager>(*gVFS, *gRenderer);
        
        // TODO Make shader manager use refreneces not pointers
        gShaderManager = std::make_unique<CE::Assets::Shaders::ShaderManager>(gRenderer.get(), gVFS.get(), *gTextureManager);;
        gMaterialManager = std::make_unique<CE::Renderer::Resources::MaterialManager>(*gTextureManager, *gRenderer);
        gGPUMeshManager = std::make_unique<CE::Renderer::Resources::GPUMeshManager>(*gRenderer, *gMaterialManager);
        gAnimatedTextureManager = std::make_unique<CE::Assets::Animations::AnimationManager>(*gVFS, *gRenderer, gInstanceID);
    }
}