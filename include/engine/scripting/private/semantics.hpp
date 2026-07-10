#pragma once

#include "engine/common/fs/vfs.hpp"

namespace CE::Scripting::Impl::Semantics {
    class SymanticAnalyser {
        public:
            SymanticAnalyser(VFS::VFS& vfs);
            
        private:
            VFS::VFS& mVFS;
    };
}
