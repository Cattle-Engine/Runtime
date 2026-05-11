#pragma once

#include "engine/audio/audio.hpp"

namespace CE::Assets::Audio {
    class AudioManager {
        public:
        AudioManager(Core::Audio::AudioSystem& audio_system,
                    VFS::VFS& vfs);

        private:
        VFS::VFS& mVFS;
        Core::Audio::AudioSystem& mAudioSys;
    };
}