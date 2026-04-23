#include <cstdlib>
#include <string>
#include <format>

#include "engine/common/error_box.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/platforms/windows.hpp"

namespace CE::Platforms::Windows {
    std::string GetCachePath(const char* game_name) {
        const char* local_app_data_str = std::getenv("LOCALAPPDATA");

        if(!local_app_data_str) {
            CE::Log(LogLevel::Fatal, "[Windows] Failed to get local appdata directory");
            ShowError("[Winows] Failed to find local appdata directory");
            std::exit(5);
        }

        std::string base_cache = std::format("{}/{}/cache", local_app_data_str, game_name);
        return base_cache;
    }

    std::string GetConfigPath(const char* game_name) {
        const char* app_data_str = std::getenv("APPDATA");

        if(!app_data_str) {
            CE::Log(LogLevel::Fatal, "[Windows] Failed to get user home directory");
            ShowError("[Winows] Failed to find user home directory");
            std::exit(5);
        }
        std::string base_config = std::format("{}/{}/config", app_data_str, game_name);
        return base_config;
    }

    std::string GetSavePath(const char* game_name) {
        const char* local_app_data_str = std::getenv("LOCALAPPDATA");
        std::string save_path = std::format("{}/{}/saves", local_app_data_str, game_name);
        return save_path;
    }
}
