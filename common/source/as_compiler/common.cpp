#include "ce_common/as_compiler/common.hpp"

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

    std::string Import2Path(const AST::ASTImport& import, const std::vector<std::filesystem::path>& import_paths) {
        namespace FS = std::filesystem;

        const size_t module_components =
            import.Symbol.has_value() && !import.Path.empty()
                ? import.Path.size() - 1
                : import.Path.size();

        FS::path relative_path;
        for (size_t i = 0; i < module_components; ++i) {
            relative_path /= import.Path[i];
        }

        const std::vector<std::string> extensions = {".as", ".ceas"};
        std::vector<FS::path> candidates;

        if (import.Symbol.has_value()) {
            for (const auto& ext : extensions) {
                candidates.push_back(relative_path / (*import.Symbol + ext));
            }

            for (const auto& ext : extensions) {
                candidates.push_back(relative_path.string() + ext);
            }

            for (const auto& ext : extensions) {
                candidates.push_back(relative_path / ("module" + ext));
            }
        } else {
            for (const auto& ext : extensions) {
                candidates.push_back(relative_path.string() + ext);
            }

            for (const auto& ext : extensions) {
                candidates.push_back(relative_path / ("module" + ext));
            }
        }

        for (const auto& root : import_paths) {
            for (const auto& candidate : candidates) {
                const FS::path resolved = root / candidate;
                if (FS::is_regular_file(resolved)) {
                    return resolved.lexically_normal().string();
                }
            }
        }

        // Preserve support for imports resolved directly from the working directory.
        for (const auto& candidate : candidates) {
            if (FS::is_regular_file(candidate)) {
                return candidate.lexically_normal().string();
            }
        }

        return "";
    }
} // namespace CE::Scripting::Impl::Common
