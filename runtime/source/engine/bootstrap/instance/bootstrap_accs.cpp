#include <memory>
#include "engine/accs/components/2d_rendering.hpp"
#include "engine/accs/components/transform.hpp"
#include "engine/accs/system.hpp"
#include "engine/accs/systems/mesh_renderer.hpp"
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
        mACCS->system_manager.RegisterSystem<ACCS::Systems::MeshRenderer>(ACCS::FrameUpdateType::Render3D);

        /* Test code
        static auto actor = mACCS->world.CreateActor();
        mACCS->world.AddComponent<ACCS::Components::Transform>(actor);
        mACCS->world.AddComponent<ACCS::Components::Sprite2D>(actor);
        auto thing = mACCS->world.GetComponent<ACCS::Components::Sprite2D>(actor);

        thing->texture = mTextureManager->Load("welcome.gif");
        thing->texture_height = thing->texture.GetTextureHeight();
        thing->texture_width = static_cast<float>(thing->texture.GetTextureWidth());
        */

        return 0;
    }
}