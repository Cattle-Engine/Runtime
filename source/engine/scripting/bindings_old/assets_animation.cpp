#include <cstdint>

#include "engine/assets/animated_textures.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

namespace CE::Scripting {
    bool Runtime::RegisterAssetAnimationBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Animations");
        CE_REGISTER_GLOBAL("void LoadAnimation(const string &in path, const string &in name)", LoadAnimation);
        CE_REGISTER_GLOBAL("void UnloadAnimation(const string &in name)", UnloadAnimation);
        CE_REGISTER_GLOBAL("uint CreateInstance(const string &in name)", CreateAnimationInstance);
        CE_REGISTER_GLOBAL("void DeleteInstance(uint handle)", DeleteAnimationInstance);
        CE_REGISTER_GLOBAL("void Play(uint handle, int x, int y, bool loop = false, bool autoRender = true)",
                           PlayAnimation);
        CE_REGISTER_GLOBAL("void PlayRot(uint handle, int x, int y, bool loop, float rotation, bool autoRender = true)",
                           PlayAnimationRot);
        CE_REGISTER_GLOBAL("void SetPosition(uint handle, int x, int y, float rotation = 0.0f)", SetAnimationPosition);
        CE_REGISTER_GLOBAL("void Seek(uint handle, uint frame)", SeekAnimation);
        CE_REGISTER_GLOBAL("void SetDrawMode(uint handle, bool autoRender)", SetAnimationDrawMode);
        CE_REGISTER_GLOBAL("void SetLooping(uint handle, bool loop)", SetAnimationLooping);
        CE_REGISTER_GLOBAL("void SetTint(uint handle, const CE::Graphics::Colour &in colour)", SetAnimationTint);
        CE_REGISTER_GLOBAL("void Pause(uint handle)", PauseAnimation);
        CE_REGISTER_GLOBAL("void Stop(uint handle)", StopAnimation);
        CE_REGISTER_GLOBAL("void DrawFrame(uint handle)", DrawAnimationFrame);

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    void Runtime::LoadAnimation(const std::string& path, const std::string& name) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Load(name, path);
    }

    void Runtime::UnloadAnimation(const std::string& name) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Unload(name);
    }

    uint32_t Runtime::CreateAnimationInstance(const std::string& name) {
        if (!mAnimationManager) {
            return 0;
        }
        return mAnimationManager->CreateInstance(name);
    }

    void Runtime::DeleteAnimationInstance(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->DeleteInstance(handle);
    }

    void Runtime::PlayAnimation(uint32_t handle, int x, int y, bool loop, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Play(handle, x, y, loop, autoRender);
    }

    void Runtime::PlayAnimationRot(uint32_t handle, int x, int y, bool loop, float rotation, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->PlayRot(handle, x, y, loop, rotation, autoRender);
    }

    void Runtime::SetAnimationPosition(uint32_t handle, int x, int y, float rotation) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetPosition(handle, x, y, rotation);
    }

    void Runtime::SeekAnimation(uint32_t handle, uint32_t frame) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Seek(handle, frame);
    }

    void Runtime::SetAnimationDrawMode(uint32_t handle, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetDrawMode(handle, autoRender);
    }

    void Runtime::SetAnimationLooping(uint32_t handle, bool loop) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetLooping(handle, loop);
    }

    void Runtime::SetAnimationTint(uint32_t handle, const Renderer::Colour& colour) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetTint(handle, colour);
    }

    void Runtime::PauseAnimation(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Pause(handle);
    }

    void Runtime::StopAnimation(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Stop(handle);
    }

    void Runtime::DrawAnimationFrame(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->DrawFrame(handle);
    }
} // namespace CE::Scripting
