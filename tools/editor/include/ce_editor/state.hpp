#pragma once

#include "ce_runtime/common/fs/vfs.hpp"

namespace CE::Editor {
    class EditorInstance;

    // state is stored on disk as TDF archive
    class State {
        public:
            State(Common::FS::VFS::VFS& vfs, EditorInstance& editor_instance);

            
        private:
            EditorInstance& mInstance;
            Common::FS::VFS::VFS& mVfs;
    };
}