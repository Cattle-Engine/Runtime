#include "engine/scripting/private/common.hpp"

namespace CE::Scripting::Impl::Common {
    std::string Import2Path(const AST::ASTImport& import, VFS::VFS& vfs) {
        std::string path;

        for (const auto& component : import.Path) {
            if (!path.empty()) {
                path += "/";
            }
            path += component;
        }

        if (vfs.FileExists((path + ".ceas").c_str())) {
            return path + ".ceas";
        } else if (vfs.FileExists((path + "/module.ceas").c_str())) {
            return path + "/module.ceas";
        } else {
            return "";
        }
    }
}