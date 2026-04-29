#pragma once

#include <memory>

#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/settings.hpp"
#include "engine/assets/fonts.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<VFS::VFS>& vfs, const char* datafilename, bool debugmode);
    int Init_GameInfo(std::unique_ptr<VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode);
    int Init_Video(std::unique_ptr<GameInfo>& gameinfo, const Settings::SettingsInfo& settings, bool debugvideo,
        std::unique_ptr<CE::Renderer::IRenderer>& renderer, RendererBackend& backend, SDL_Window*& window,
        std::unique_ptr<VFS::VFS>& vfs, Renderer::GPUDeviceHandle gpudevice);
    int Init_AssetManagers(std::unique_ptr<CE::Assets::Textures::TextureManager>& texturemanager_ptr, std::unique_ptr<VFS::VFS>& vfs_ptr, 
        std::unique_ptr<CE::Renderer::IRenderer>& renderer, std::unique_ptr<CE::Assets::Fonts::FontManager>& font_manager_ptr, int);
}
