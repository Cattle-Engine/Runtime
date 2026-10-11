#include "engine/scripting/private/common.hpp"

#include <fstream>
#include <sstream>

namespace CE::Scripting::Impl::Common {

    std::string GetScriptFromFS(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return {};
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    std::string Import2Path(const AST::ASTImport& import) {
        size_t module_components =
            import.Symbol.has_value() && !import.Path.empty() ? import.Path.size() - 1 : import.Path.size();

        std::string path;
        for (size_t i = 0; i < module_components; ++i) {
            if (!path.empty())
                path += "/";
            path += import.Path[i];
        }

        const std::vector<std::string> exts = {".as", ".ceas"};

        if (import.Symbol.has_value()) {
            for (const auto& ext : exts) {
                std::string file = path.empty() ? (*import.Symbol + ext) : (path + "/" + *import.Symbol + ext);
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }

            for (const auto& ext : exts) {
                std::string file = path + ext;
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }

            for (const auto& ext : exts) {
                std::string file = path + "/module" + ext;
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }
        } else {
            for (const auto& ext : exts) {
                std::string file = path + ext;
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }
            for (const auto& ext : exts) {
                std::string file = path + "/module" + ext;
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }
        }

        return "";
    }
} // namespace CE::Scripting::Impl::Common
