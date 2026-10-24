#include <memory>
#include "engine/accs/system.hpp"
#include "engine/instance.hpp"

#include "engine/accs/systems/sprite_renderer.hpp"

namespace CE {
    int Instance::Bootstrap_ACCS() {
        mACCS = std::make_unique<ACCS::ACCS>(ACCS::Dependencies{
            *this,
            *mGameInfo,
            *mVFS,
            mGameStateManager,
            *mSettingsManager,
            *mRenderer,
            *mTextureManager,
            *mShaderManager,
            *mMaterialManager,
            *mGPUMeshManager,
            *mModelRenderer,
            *mFontManager,
            *m3DModelImporter,
            *mAnimatedTextureManager,
            *mMouseManger,
            *mKeyboardManger,
            *mTextInputManager,
            *mInputBindingManager,
            mAudioManager.get(),
            mAudioSystem.get(),
            *mWindow,
            *mRendererResourcesNameRegistry
        });

        mACCS->system_manager.RegisterSystem<ACCS::Systems::SpriteRenderer>(ACCS::FrameUpdateType::Render2D);

        return 0;
    }
}