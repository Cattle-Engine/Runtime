#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Assets::Animations {
    struct FrameInfo {
        uint32_t Width;
        uint32_t Height;
        uint32_t X;
        uint32_t Y;
        uint32_t Duration;
    };

    struct AnimationInfo {
        std::string mSourceFileName;
        uint32_t FrameCount;
        Renderer::Texture* Texture;
        std::vector<FrameInfo> FramesInfo;
    };

    struct AnimationInstance {
        uint32_t CurrentFrame;
        AnimationInfo* AnimInfo;
        bool IsPlaying;
        bool Loop;
        bool AutoRender;
        int X, Y;
        float FrameTimer;
        float Rotation;
        Renderer::Colour Tint = {255, 255, 255, 255};
    };

    struct AnimationInstanceHandle {
        uint32_t id;

        bool operator==(const AnimationInstanceHandle& o) const {
            return id == o.id;
        }
    };

    struct AnimationInstanceHandleHash {
        std::size_t operator()(const AnimationInstanceHandle& s) const noexcept {
            return std::hash<uint32_t>{}(s.id);
        }
    };

    class AnimatedTextureManager {
      public:
        AnimatedTextureManager(Common::FS::VFS::VFS& vfs, Renderer::IRenderer& renderer, int instance_id);

        // path must be a tdf file on VFS
        bool Load(std::string name, std::string path);
        bool Unload(std::string name);
        AnimationInstanceHandle CreateInstance(std::string name);
        bool DeleteInstance(AnimationInstanceHandle handle);

        void Play(AnimationInstanceHandle handle, int x, int y, bool loop, bool auto_render);
        void PlayRot(AnimationInstanceHandle handle, int x, int y, bool loop, float rotation, bool auto_render);
        void SetPosition(AnimationInstanceHandle handle, int x, int y, float rotation);
        void Seek(AnimationInstanceHandle handle, uint32_t frame);
        void SetDrawMode(AnimationInstanceHandle handle, bool auto_render);
        void SetLooping(AnimationInstanceHandle handle, bool loop);
        void SetTint(AnimationInstanceHandle handle, Renderer::Colour colour);
        // stops an animation playing and keeps the current playback position
        void Pause(AnimationInstanceHandle handle);
        // stops an animation and does not keep the current playback position
        void Stop(AnimationInstanceHandle handle);

        void DrawFrame(AnimationInstanceHandle handle);

        void Update(float dt);
        void Render();

      private:
        AnimationInstance* GetAnimationInfo(AnimationInstanceHandle handle);
        std::unordered_map<std::string, std::shared_ptr<AnimationInfo>> mAnimations;
        std::unordered_map<AnimationInstanceHandle, AnimationInstance, AnimationInstanceHandleHash> mAnimationInstances;
        Common::FS::VFS::VFS& mVFS;
        Renderer::IRenderer& mRenderer;
        uint32_t mNextHandleID;
        int mInstanceID;
    };
} // namespace CE::Assets::Animations
