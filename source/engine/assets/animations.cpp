#include "engine/assets/animations.hpp"
#include "engine/common/fs/tdf.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Animations {
    AnimationManager::AnimationManager(VFS::VFS& vfs, Renderer::IRenderer& renderer, int instance_id)
    : mVFS(vfs), mRenderer(renderer) {
        mInstanceID = instance_id;
    }

    AnimationInstance* AnimationManager::GetAnimationInfo(uint32_t handle) {
        auto it = mAnimationInstances.find(handle);
        if (it == mAnimationInstances.end()) {
            CE::Log(LogLevel::Error, "[Animation Manager {}] Use after free or invalid handle: {}", mInstanceID, handle);
            return nullptr;
        } else {
            return &it->second;
        }
    }

    void AnimationManager::Load(std::string name, std::string path) {
        if (!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing animation info file: {}",
                mInstanceID, path);
            return;
        }

        TDFFile info;
        info.load(mVFS, path);

        if (!info.has("SourceImagePath")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing source image path key in: {}",
                mInstanceID, path);
            return;
        }

        std::string source_image_path =
            TDFFile::readString(info.entries["SourceImagePath"]);

        if (!mVFS.FileExists(source_image_path.c_str())) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing source image file: {}",
                mInstanceID, source_image_path);
            return;
        }

        if (!info.has("FrameCount")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing frame count key in: {}",
                mInstanceID, path);
            return;
        }

        uint32_t frame_count =
            TDFFile::readUInt(info.entries["FrameCount"]);

        if (!info.has("Frames")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing Frames array in: {}",
                mInstanceID, path);
            return;
        }

        std::vector<TDFFile> frames =
            TDFFile::readObjectArray(info.entries["Frames"]);

        AnimationInfo anim;
        anim.mSourceFileName = source_image_path;
        anim.Texture = mRenderer.LoadTex(source_image_path.c_str());
        anim.FramesInfo.reserve(frame_count);
        anim.FrameCount = frame_count;
        for (const TDFFile& f : frames) {
            FrameInfo frame{};

            if (f.has("Width"))    frame.Width    = TDFFile::readUInt(f.entries.at("Width"));
            if (f.has("Height"))   frame.Height   = TDFFile::readUInt(f.entries.at("Height"));
            if (f.has("X"))        frame.X        = TDFFile::readUInt(f.entries.at("X"));
            if (f.has("Y"))        frame.Y        = TDFFile::readUInt(f.entries.at("Y"));
            if (f.has("Duration")) frame.Duration  = TDFFile::readUInt(f.entries.at("Duration"));

            anim.FramesInfo.push_back(frame);
        }

        mAnimations[name] = std::move(anim);
    }

    uint32_t AnimationManager::CreateInstance(std::string name) {
        auto it = mAnimations.find(name);

        if(it != mAnimations.end()) {
            AnimationInstance instance = {};
            instance.IsPlaying = false;
            instance.CurrentFrame = 0;
            instance.X = 0;
            instance.Y = 0;
            instance.Rotation = 0.0f;
            instance.FrameTimer = 0.0f;
            instance.AnimInfo = &it->second;
            uint32_t handle = mNextHandleID++;
            mAnimationInstances[handle] = std::move(instance);
            return handle;
        }
        CE::Log(LogLevel::Error, "[Animation Manager {}] Tried using an unloaded or missing animation: {}", 
        mInstanceID, name);
        return 0;
    }

    void AnimationManager::Play(uint32_t handle, int x, int y, bool loop, bool auto_render) {
        this->PlayRot(handle, x, y, loop, 0.0f, auto_render);
    }

    void AnimationManager::PlayRot(uint32_t handle, int x, int y, bool loop, float rotation,  bool auto_render) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;

        info->X = x;
        info->Y = y;
        info->Rotation = rotation;
        info->Loop = loop;
        info->AutoRender = auto_render;
        info->CurrentFrame = 0;
        info->IsPlaying = true;
        info->FrameTimer = 0.0f;
    }

    void AnimationManager::SetPosition(uint32_t handle, int x, int y, float rotation) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;

        info->X = x;
        info->Y = y;
        info->Rotation = rotation;
    }

    void AnimationManager::Seek(uint32_t handle, uint32_t frame) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;

        if (frame >= info->AnimInfo->FrameCount) {
            CE::Log(LogLevel::Error, "[Animation Manager {}] Frame is out of range: {} (max: {})",
            mInstanceID, frame, info->AnimInfo->FrameCount);
            return;
        }
        info->CurrentFrame = frame;
    }

    void AnimationManager::SetLooping(uint32_t handle, bool loop) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;

        info->Loop = loop;
    }

    void AnimationManager::Pause(uint32_t handle) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;
        
        info->IsPlaying = false;
    }

    void AnimationManager::SetTint(uint32_t handle, Renderer::Colour colour) {
        auto info = GetAnimationInfo(handle);
        if(info == nullptr) return;

        info->Tint = colour;
    }

    void AnimationManager::Stop(uint32_t handle) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;
        
        info->CurrentFrame = 0;
        info->IsPlaying = false;
    }

    void AnimationManager::SetDrawMode(uint32_t handle, bool auto_render) {
        auto info = GetAnimationInfo(handle);
        if (info == nullptr) return;

        info->AutoRender = auto_render;
    }

    void AnimationManager::Update(float dt) {
        for (auto& [handle, anim] : mAnimationInstances) {

            if (!anim.IsPlaying || anim.AnimInfo == nullptr)
                continue;

            if (anim.CurrentFrame >= anim.AnimInfo->FrameCount)
                anim.CurrentFrame = 0;

            const FrameInfo& frame =
                anim.AnimInfo->FramesInfo[anim.CurrentFrame];

            anim.FrameTimer += dt;
            float frameDuration =
                static_cast<float>(frame.Duration) / 1000.0f;
            while (anim.FrameTimer >= frameDuration) {
                anim.FrameTimer -= frameDuration;
                anim.CurrentFrame++;
                if (anim.CurrentFrame >= anim.AnimInfo->FrameCount) {

                    if (anim.Loop) {
                        anim.CurrentFrame = 0;
                    } else {
                        anim.CurrentFrame =
                            anim.AnimInfo->FrameCount - 1;

                        anim.IsPlaying = false;
                        break;
                    }
                }
                frameDuration =
                    static_cast<float>(
                        anim.AnimInfo
                            ->FramesInfo[anim.CurrentFrame]
                            .Duration
                    ) / 1000.0f;
            }
        }
    }

    void AnimationManager::Render() {
        for (auto& [handle, anim] : mAnimationInstances) {

            if (anim.AnimInfo == nullptr)
                continue;

            if (!anim.AutoRender) continue;

            const FrameInfo& frame =
                anim.AnimInfo->FramesInfo[anim.CurrentFrame];

            CE::Renderer::Texture* tex = anim.AnimInfo->Texture;

            if (tex == nullptr)
                continue;

            float texWidth  = static_cast<float>(tex->width);
            float texHeight = static_cast<float>(tex->height);

            float u0 = frame.X / texWidth;
            float v0 = frame.Y / texHeight;

            float u1 = (frame.X + frame.Width) / texWidth;
            float v1 = (frame.Y + frame.Height) / texHeight;

            mRenderer.DrawTexUV(
                tex,
                static_cast<float>(anim.X),
                static_cast<float>(anim.Y),
                static_cast<float>(frame.Width),
                static_cast<float>(frame.Height),
                u0,
                v0,
                u1,
                v1,
                anim.Tint,
                anim.Rotation
            );
        }
    }

    void AnimationManager::DeleteInstance(uint32_t handle) {
        auto it = mAnimationInstances.find(handle);

        if (it == mAnimationInstances.end()) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Tried to delete invalid handle: {}",
                mInstanceID, handle);
            return;
        }

        mAnimationInstances.erase(it);
    }

    void AnimationManager::Unload(std::string name) {
        auto it = mAnimations.find(name);

        if (it == mAnimations.end()) return;
        for (auto instIt = mAnimationInstances.begin();
            instIt != mAnimationInstances.end(); ) {

            if (instIt->second.AnimInfo == &it->second) {
                instIt = mAnimationInstances.erase(instIt);
            } else {
                ++instIt;
            }
        }
        mRenderer.UnloadTex(it->second.Texture);
        mAnimations.erase(it);
    }

    void AnimationManager::DrawFrame(uint32_t handle) {
        auto it = mAnimationInstances.find(handle);

        if (it->second.AnimInfo == nullptr) return;

        const FrameInfo& frame = it->second.AnimInfo->FramesInfo[it->second.CurrentFrame];

        CE::Renderer::Texture* tex = it->second.AnimInfo->Texture;

        if (tex == nullptr) return;

        float texWidth  = static_cast<float>(tex->width);
        float texHeight = static_cast<float>(tex->height);

        float u0 = frame.X / texWidth;
        float v0 = frame.Y / texHeight;

        float u1 = (frame.X + frame.Width) / texWidth;
        float v1 = (frame.Y + frame.Height) / texHeight;

        mRenderer.DrawTexUV(
            tex,
            static_cast<float>(it->second.X),
            static_cast<float>(it->second.Y),
            static_cast<float>(frame.Width),
            static_cast<float>(frame.Height),
            u0,
            v0,
            u1,
            v1,
            it->second.Tint,
            it->second.Rotation
        );
    }
}
