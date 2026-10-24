#include <memory>

#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_RendererResourceManagers() {
        mTextureManager = std::make_unique<CE::Renderer::Resources::TextureManager>(*mVFS, *mRenderer);
        mFontManager = std::make_unique<CE::Assets::Fonts::FontManager>(*mRenderer, *mVFS, mInstanceID);

        mShaderManager = std::make_unique<CE::Renderer::Resources::ShaderManager>(*mVFS, *mRenderer, *mTextureManager);
        mMaterialManager = std::make_unique<CE::Renderer::Resources::MaterialManager>(*mTextureManager, *mRenderer, *mShaderManager);
        mGPUMeshManager = std::make_unique<CE::Renderer::Resources::GPUMeshManager>(*mRenderer, *mMaterialManager);
        mAnimatedTextureManager =
            std::make_unique<CE::Assets::Animations::AnimatedTextureManager>(*mVFS, *mRenderer, mInstanceID);
        mModelRenderer =
            std::make_unique<CE::Renderer::Resources::ModelRenderer>(*mMaterialManager, *mGPUMeshManager, *mRenderer);

        mRendererResourcesNameRegistry = std::make_unique<CE::Common::Containers::RendererResourcesNameRegistry>();

        return 0;
    }
} // namespace CE