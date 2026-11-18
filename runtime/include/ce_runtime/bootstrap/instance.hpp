#pragma once

#include <memory>

#include "ce_runtime/assets/fonts.hpp"
#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/rendering/renderer.hpp"
#include "ce_runtime/rendering/resources/shader_manager.hpp"
#include "ce_runtime/settings.hpp"

namespace CE::Bootstrap {
    int Init_GameInfo(std::unique_ptr<Common::FS::VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode);
} // namespace CE::Bootstrap
