#pragma once 

#include "engine/common/fs/vfs.hpp"

#include <SDL3_mixer/SDL_mixer.h>

namespace CE::Core::Audio {
    struct Music {

    };
    
    class AudioSystem {
        public:
            AudioSystem(VFS::VFS& vfs);
        private:

    };
}