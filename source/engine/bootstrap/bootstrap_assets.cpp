#include <memory>

#include "engine/bootstrap/instance.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/assets/textures.hpp"
#include "engine/common/vfs.hpp"

namespace CE::Bootstrap {
    int Init_AssetManagers(std::unique_ptr<CE::Assets::Textures::TextureManager>& texturemanager_ptr, std::unique_ptr<VFS::VFS>& vfs_ptr, 
        std::unique_ptr<CE::Renderer::IRenderer>& renderer, std::unique_ptr<CE::Assets::Fonts::FontManager>& font_manager_ptr, int instance_id) {
        texturemanager_ptr = std::make_unique<CE::Assets::Textures::TextureManager>(
            renderer.get(), vfs_ptr.get()
        );

        font_manager_ptr = std::make_unique<CE::Assets::Fonts::FontManager> (
            *renderer, *vfs_ptr, instance_id
        );

        return 0;
    }    
}