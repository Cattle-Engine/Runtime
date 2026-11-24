#include "ce_editor/undoredo_system.hpp"
#include <yaml-cpp/emittermanip.h>
#include "yaml-cpp/yaml.h"

#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_editor/project.hpp"
#include <exception>
#include <filesystem>
#include <stdexcept>

namespace CE::Editor {
    namespace {
        template <typename T>
        T Required(const YAML::Node& node, const char* key) {
            const YAML::Node v = node[key];
            if (!v) {
                throw std::runtime_error(std::string("missing required key '") + key + "'");
            }
            return v.as<T>();
        }

        template <typename T>
        T Optional(const YAML::Node& node, const char* key, T fallback) {
            const YAML::Node v = node[key];
            return v ? v.as<T>() : fallback;
        }

        std::string ToLower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        CE::Common::WindowMode ParseWindowMode(const std::string& text) {
            const std::string s = ToLower(text);
            if (s == "fullscreen") return CE::Common::WindowMode::Fullscreen;
            if (s == "borderless") return CE::Common::WindowMode::Borderless;
            if (s == "windowed") return CE::Common::WindowMode::Windowed;
            throw std::runtime_error("window_mode must be fullscreen, borderless or windowed");
        }

        const char* WindowModeName(CE::Common::WindowMode mode) {
            switch (mode) {
                case CE::Common::WindowMode::Fullscreen: return "fullscreen";
                case CE::Common::WindowMode::Borderless: return "borderless";
                default: return "windowed";
            }
        }

        CE::GameInfo ParseGameInfo(const YAML::Node& n) {
            CE::GameInfo g{};

            g.game_name = Required<std::string>(n, "game_name");
            g.game_version = Required<std::string>(n, "game_version");
            g.game_package_id = Required<std::string>(n, "package_id");
            g.startupFileName = Required<std::string>(n, "startup_file");

            g.windowWidth = Required<int>(n, "window_width");
            g.windowHeight = Required<int>(n, "window_height");
            g.windowTitle = Required<std::string>(n, "window_title");
            g.maxFPS = Required<int>(n, "max_fps");
            g.rendererName = Required<std::string>(n, "renderer");
            g.enableVSync = Required<bool>(n, "vsync");
            g.windowMode = ParseWindowMode(Required<std::string>(n, "window_mode"));
            g.resizableWindow = Required<bool>(n, "resizable");

            g.windowIcon = Optional<std::string>(n, "window_icon", "");
            g.pauseRenderingWhenFocusLostInWindowedMode = Optional<bool>(n, "no_render_on_focus_lost", false);
            g.pauseUpdateWhenFocusLost = Optional<bool>(n, "no_update_on_focus_lost", false);
            g.minWindowWidth = Required<int>(n, "min_window_width");
            g.minWindowHeight = Required<int>(n, "min_window_height");
            g.maxWindowWidth = Required<int>(n, "max_window_width");
            g.maxWindowHeight = Required<int>(n, "max_window_height");

            return g;
        }

        // Returns an empty string if valid, otherwise a description of the problem.
        std::string ValidateGameInfo(const CE::GameInfo& g) {
            const std::string& id = g.game_package_id;
            if (id.empty() || id.front() == '.' || id.back() == '.' ||
                id.find("..") != std::string::npos) {
                return "package_id must not be empty, start/end with '.', or contain '..'";
            }
            for (unsigned char c : id) {
                if (!std::isalnum(c) && c != '.' && c != '_' && c != '-') {
                    return "package_id may only contain ASCII letters, digits, '.', '_' and '-'";
                }
            }

            if (g.game_name.empty()) return "game_name must not be empty";
            if (g.game_version.empty()) return "game_version must not be empty";
            if (g.startupFileName.empty()) return "startup_file must not be empty";
            if (g.windowWidth <= 0 || g.windowHeight <= 0) return "window size must be positive";
            if (g.maxFPS <= 0) return "max_fps must be positive";

            const std::string r = ToLower(g.rendererName);
            if (r != "vulkan" && r != "software") return "renderer must be Vulkan or Software";

            if (g.minWindowWidth < 0 || g.minWindowHeight < 0 ||
                g.maxWindowWidth < 0 || g.maxWindowHeight < 0) {
                return "min/max window sizes must not be negative";
            }
            if (g.maxWindowWidth && g.minWindowWidth > g.maxWindowWidth) return "min_window_width > max_window_width";
            if (g.maxWindowHeight && g.minWindowHeight > g.maxWindowHeight) return "min_window_height > max_window_height";

            return {};
        }
    }
    
    Project::Project(fs::path proj_path) : mProjectPath(proj_path) {
        const fs::path manifest_path = mProjectPath / kProjectManifestName;
        if (!fs::exists(manifest_path) || !fs::is_regular_file(manifest_path)) {
            throw std::runtime_error(
                "Project manifest: '" + manifest_path.string()  + "' does not exist, or is not regular file"
            );
        }
        
        const YAML::Node m = YAML::LoadFile(manifest_path);
        const YAML::Node gi = m["gameinfo"];
        if (!gi || !gi.IsMap()) throw std::runtime_error("Missing 'gameinfo' section in: " + manifest_path.string());

        mGameInfo = ParseGameInfo(gi);
        std::string gi_error = ValidateGameInfo(mGameInfo);
        if (!gi_error.empty()) throw std::runtime_error("Gameinfo validation error: " + gi_error);
    }

    fs::path Project::GetProjectPath() const {
        return mProjectPath;
    }

    UndoRedoSystem& Project::GetUndoRedoSys() const { return mUndoRedo; }
}