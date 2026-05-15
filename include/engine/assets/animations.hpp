#pragma once

#include "engine/common/fs/vfs.hpp"
#include "engine/renderer.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

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
        int FrameCount;
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
    
    class AnimationManager {
        public:
        AnimationManager(VFS::VFS& vfs, Renderer::IRenderer& renderer, int instance_id);

        void Load(std::string name, std::string path);
        void Unload(std::string name);
        uint32_t CreateInstance(std::string name);
        void DeleteInstance(uint32_t handle);

        void Play(uint32_t handle, int x, int y, bool loop, bool auto_render);
        void PlayRot(uint32_t handle, int x, int y, bool loop, float rotation, bool auto_render);
        void SetPosition(uint32_t handle, int x, int y, float rotation);
        void Seek(uint32_t handle, uint32_t frame);
        void SetDrawMode(uint32_t handle,bool auto_render);
        void SetLooping(uint32_t handle, bool loop);
        void SetTint(uint32_t handle, Renderer::Colour colour);
        void Pause(uint32_t handle);
        void Stop(uint32_t handle);
        void DrawFrame(uint32_t handle);

        void Update(float dt);
        void Render();
        private:
        AnimationInstance* GetAnimationInfo(uint32_t handle);
        std::unordered_map<std::string, AnimationInfo> mAnimations;
        std::unordered_map<uint32_t, AnimationInstance> mAnimationInstances; 
        VFS::VFS& mVFS;
        Renderer::IRenderer& mRenderer;
        uint32_t mNextHandleID;
        int mInstanceID;
    };
}