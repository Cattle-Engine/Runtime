#include "engine/scripting/private/common.hpp"

#include <algorithm>

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
        size_t module_components =
            import.Symbol.has_value() && !import.Path.empty() ? import.Path.size() - 1 : import.Path.size();

        std::string path;
        for (size_t i = 0; i < module_components; ++i) {
            if (!path.empty())
                path += "/";
            path += import.Path[i];
        }

        if (import.Symbol.has_value()) {
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path.empty() ? (*import.Symbol + ext) : (path + "/" + *import.Symbol + ext);
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }

            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }

            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path + "/module" + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
        } else {
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path + "/module" + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
        }

        return "";
    }
} // namespace CE::Scripting::Impl::Common
