#include "engine/scripting/angelscript.hpp"

namespace  CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path) {
        VirtualFile* f = vfs.OpenFile(path);
        if (!f) return "";

        uint64_t size = 0;
        vfs.GetFileSize(path, size);

        std::string out;
        out.resize(size);

        vfs.ReadFile(f, out.data(), size);
        vfs.CloseFile(f);

        return out;
    }
}

