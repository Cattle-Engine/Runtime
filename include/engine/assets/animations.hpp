#pragma once

#include "engine/common/fs/vfs.hpp"
#include "engine/renderer.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace CE::Assets::Animations {
    class AnimationManager {
        public:
        AnimationManager(VFS::VFS& vfs, Renderer::IRenderer& renderer);

        private:
        struct FrameInfo {
            int Width;
            int Height;
            int X;
            int Y;
            int Duration;
        };

        struct AnimationInfo {
            std::string mSourceFileName;
            bool Loop;
            std::vector<FrameInfo> FramesInfo;
        };
        VFS::VFS& mVFS;
        Renderer::IRenderer& mRenderer;
    };
}