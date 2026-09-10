#pragma once

#include <memory>

#include "engine/assets/fonts.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/settings.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<VFS::VFS>& vfs, const char* datafilename, bool debugmode);
    int Init_GameInfo(std::unique_ptr<VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode);
} // namespace CE::Bootstrap
