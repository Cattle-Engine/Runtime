#pragma once

#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_common/result.hpp"

namespace CE::Editor {
    class EditorInstance;

    // state is stored on disk as TDF archive
    class State {
        public:
            State(Common::FS::VFS::VFS& vfs, EditorInstance& editor_instance);

            // reads mInstance and saves internal stuff such as window size 
            Status SaveCurrentState();
            // reads the state from disk, does not modify the instance state if it fails to get state from disk
            Status RestoreState();
        private:
            EditorInstance& mInstance;
            Common::FS::VFS::VFS& mVFS;
    };
}