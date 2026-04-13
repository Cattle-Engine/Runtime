#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "engine/common/vfs.hpp"
#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/bootstrap.hpp"
#include "engine/gameinfo.hpp"

namespace CE {
    class Instance {
        public:
            Instance(const char* data_file_name, bool debugmode);
            ~Instance();
        private:
            std::unique_ptr<CE::VFS::VFS> gVFS;
            std::unique_ptr<CE::GameInfo> gGameInfo;
            SDL_Window* gWindow = nullptr;
            CE::Renderer::IRenderer* gRenderer = nullptr;
            CE::Assets::Textures::TextureManager* gTextureManager = nullptr;
            RendererBackend gRendererBackend = RendererBackend::None;
            bool gDebug;
            bool shouldExit;
    };
}