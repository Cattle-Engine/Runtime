#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "engine/common/vfs.hpp"
#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/gameinfo.hpp"

// A global to get all instances
inline Uint64 GLOBALINSTANCESCOUNTER;

namespace CE {
    class Instance {
        public:
            Instance(const char* data_file_path, bool debugmode, 
                Renderer::GPUDeviceHandle& gpudevice);
            int Update();
            bool ShouldExit();
            int GetInstanceID();
            ~Instance();
        private:
            std::unique_ptr<CE::VFS::VFS> gVFS;
            std::unique_ptr<CE::GameInfo> gGameInfo;
            std::unique_ptr<CE::Renderer::IRenderer> gRenderer;
            std::unique_ptr<CE::Assets::Textures::TextureManager> gTextureManager;
            
            SDL_Window* gWindow = nullptr;
            RendererBackend gRendererBackend = RendererBackend::None;
            bool gDebug;
            bool gShouldExit;

            // The id for the instance
            int gInstanceID;
            // The id for the window in the instance, provided by SDL
            int gInstanceWindowID;
    };

    using InstanceHandle = std::unique_ptr<Instance>;
}