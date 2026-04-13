#include <memory>

#include "engine/bootstrap.hpp"
#include "engine/assets/textures.hpp"
#include "engine/common/vfs.hpp"

namespace CE::Bootstrap {
    int Init_AssetManagers(std::unique_ptr<CE::Assets::Textures::TextureManager>& texturemanager_ptr, std::unique_ptr<VFS::VFS>& vfs_ptr, 
        std::unique_ptr<CE::Renderer::IRenderer>& renderer) {
        texturemanager_ptr = std::make_unique<CE::Assets::Textures::TextureManager>(
            renderer, vfs_ptr
        );
        return 0;
    }    
}