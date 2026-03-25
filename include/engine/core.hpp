#pragma once

#include <string>

#include "engine/common/vfs.hpp"
#include <SDL3/SDL.h>

enum class RendererBackend {
    Software,
    OpenGL,
    DirectX,
    Metal,
    Vulkan
};

namespace CE::Core { 
    inline constexpr const char* engineVersionString = "Alpha 0.2";
    inline constexpr int engineVersionMajor = 0;
    inline constexpr int engineVersionMinor = 2;

    inline bool DebugMode = true;

    inline int renderer = 0;
    inline RendererBackend renderer = RendererBackend::Software;
}

namespace CE::Global {
    CE::VFS::VFS& GetVFS();
    void SetVFS(CE::VFS::VFS* vfs);
    inline SDL_Window* gameWindow = nullptr;
    inline void* rendererHandle = nullptr;
}

namespace CE::GameInfo {
    inline std::string gameNameString;
    inline std::string gameVersionString;

    inline std::string rendererName;
    inline int windowHeight;
    inline int windowWidth;
    inline std::string windowTitle;
    inline int maxFPS;

    inline const char* dataFileName = CE_DATA_FILE_NAME;
}