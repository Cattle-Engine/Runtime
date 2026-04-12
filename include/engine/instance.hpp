#pragma once

#include "engine/common/vfs.hpp"
#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/bootstrap.hpp"
#include "engine/gameinfo.hpp"
#include <SDL3/SDL.h>

namespace CE {
    class Instance {
        public:
            Instance(const char* data_file_name, bool debugmode);
            ~Instance();
        private:
            CE::VFS::VFS gVFS;
            SDL_Window* gWindow = nullptr;
            CE::Renderer::IRenderer* gRenderer = nullptr;
            CE::Assets::Textures::TextureManager* gTextureManager = nullptr;
            CE::GameInfo gGameInfo;
            RendererBackend gRendererBackend = RendererBackend::None;
            bool debugMode;
            bool shouldExit;
    };
}