#pragma once

#include <string>

#include "engine/common/vfs.hpp"
#include <SDL3/SDL.h>

namespace CE::Core { 
    inline constexpr const char* engineVersionString = "Alpha 0.2";
    inline constexpr int engineVersionMajor = 0;
    inline constexpr int engineVersionMinor = 2;

    inline bool debugMode = true;
}

namespace CE {
    void main();
}

namespace CE::Global {
    CE::VFS::VFS& GetVFS();
    void SetVFS(CE::VFS::VFS* vfs);
    inline SDL_Window* gameWindow = nullptr;
    inline void* rendererHandle = nullptr;
}