#include "engine/scripting/angelscript.hpp"

namespace CE::Scripting::Utils {
    std::string LoadScript(Common::FS::VFS::VFS& vfs, const char* path) {
        auto f = vfs.OpenFile(path);
        if (!f)
            return "";

        uint64_t size = 0;
        size = f->Size();

        std::string out;
        out.resize(size);

        if (size != 0 && !f->Read(out.data(), size))
            return "";

        return out;
    }
} // namespace CE::Scripting::Utils
