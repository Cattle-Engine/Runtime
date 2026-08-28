#include <memory>

#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_RendererResourceManagers() {
        gTextureManager = std::make_unique<CE::Renderer::Resources::TextureManager>(*gVFS, *gRenderer);
        gSkyBoxManager = std::make_unique<CE::Renderer::Resources::SkyBoxManager>(gRenderer.get(), gVFS.get());
        gFontManager = std::make_unique<CE::Assets::Fonts::FontManager>(*gRenderer, *gVFS, gInstanceID);

        gShaderManager = std::make_unique<CE::Renderer::Resources::ShaderManager>(*gVFS, *gRenderer, *gTextureManager);
        gMaterialManager = std::make_unique<CE::Renderer::Resources::MaterialManager>(*gTextureManager, *gRenderer);
        gGPUMeshManager = std::make_unique<CE::Renderer::Resources::GPUMeshManager>(*gRenderer, *gMaterialManager);
        gAnimatedTextureManager =
            std::make_unique<CE::Assets::Animations::AnimatedTextureManager>(*gVFS, *gRenderer, gInstanceID);
        gModelRenderer =
            std::make_unique<CE::Renderer::Resources::ModelRenderer>(*gMaterialManager, *gGPUMeshManager, *gRenderer);

        mRendererResourcesNameRegistry = std::make_unique<CE::Common::Containers::RendererResourcesNameRegistry>();

        return 0;
    }
} // namespace CE