#include <algorithm>
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
        std::string module_path = import.Module;
        std::replace(module_path.begin(), module_path.end(), ':', '/');

        if (import.Symbol.has_value()) {
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = module_path + "/" + *import.Symbol + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
            
            // if the symbol isn't a separate file, it might be in the module file
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = module_path + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
            
            // try foo/module.as or foo/module.ceas
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = module_path + "/module" + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
        } else {
            // importing whole module
            // first we try: tests.as or tests.ceas
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = module_path + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
            
            // then we try: tests/module.as or tests/module.ceas
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = module_path + "/module" + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
        }
        /*
        // probably dead code, keeping here for just in case
            std::string path;
            for (const auto& component : import.Path) {
                if (!path.empty()) {
                    path += "/";
                }
                path += component;
            }
            
            for (const auto& ext : {".as", ".ceas"}) {
                std::string file = path + ext;
                if (vfs.FileExists(file.c_str())) {
                    return file;
                }
            }
            
            return "";
        */
        return "";
    }
}
