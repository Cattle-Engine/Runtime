#include <memory>

#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_RendererResourceManagers() {
        mTextureManager = std::make_unique<CE::Renderer::Resources::TextureManager>(*mVFS, *mRenderer);
        gFontManager = std::make_unique<CE::Assets::Fonts::FontManager>(*mRenderer, *mVFS, gInstanceID);

        mShaderManager = std::make_unique<CE::Renderer::Resources::ShaderManager>(*mVFS, *mRenderer, *mTextureManager);
        mMaterialManager = std::make_unique<CE::Renderer::Resources::MaterialManager>(*mTextureManager, *mRenderer);
        mGPUMeshManager = std::make_unique<CE::Renderer::Resources::GPUMeshManager>(*mRenderer, *mMaterialManager);
        gAnimatedTextureManager =
            std::make_unique<CE::Assets::Animations::AnimatedTextureManager>(*mVFS, *mRenderer, gInstanceID);
        mModelRenderer =
            std::make_unique<CE::Renderer::Resources::ModelRenderer>(*mMaterialManager, *mGPUMeshManager, *mRenderer);

        mRendererResourcesNameRegistry = std::make_unique<CE::Common::Containers::RendererResourcesNameRegistry>();

        return 0;
    }
} // namespace CE