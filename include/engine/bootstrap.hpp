#pragma once

#include "engine/common/vfs.hpp"
#include "engine/gameinfo.hpp"
#include "engine/renderer.hpp"

namespace CE::Bootstrap {
    void Init_GameData(VFS::VFS* vfs, GameInfo* gameinfo, bool debugmode);
    void Init_GameInfo(VFS::VFS* vfs, GameInfo* gameinfo, bool debugmode);
    void Init_Video(GameInfo* gameinfo, bool debugvideo, CE::Renderer::IRenderer* renderer, RendererBackend backend, SDL_Window* window);
}
