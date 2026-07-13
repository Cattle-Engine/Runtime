#include "engine/scripting/private/common.hpp"

namespace CE::Scripting::Impl::Common {
    std::string GetScriptFromVFS(const std::string& path, VFS::VFS& vfs) {
        VirtualFile* file = vfs.OpenFile(path.c_str());
        if (file == nullptr) {
            return {};
        }

        uint64_t size = 0;
        if (!vfs.GetFileSize(path.c_str(), size)) {
            vfs.CloseFile(file);
            return {};
        }
        std::string data(size, '\0');
        const size_t read = size == 0 ? 0 : vfs.ReadFile(file, data.data(), data.size());
        vfs.CloseFile(file);
        data.resize(read);
        return data;
    }

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
