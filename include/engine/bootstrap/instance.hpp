#pragma once

#include <memory>

#include "engine/assets/fonts.hpp"
#include "engine/rendering/resources/skybox_manager.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/settings.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<VFS::VFS>& vfs, const char* datafilename, bool debugmode);
    int Init_GameInfo(std::unique_ptr<VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode);
    int Init_Video(std::unique_ptr<GameInfo>& gameinfo, const Settings::SettingsInfo& settings, bool debugvideo,
                   std::unique_ptr<CE::Renderer::IRenderer>& renderer, RendererBackend& backend, SDL_Window*& window,
                   std::unique_ptr<VFS::VFS>& vfs, Renderer::GPUDeviceHandle gpudevice);
} // namespace CE::Bootstrap
