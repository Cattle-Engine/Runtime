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
        const char* user_profile_str = std::getenv("USERPROFILE");

        if(!user_profile_str) {
            CE::Log(LogLevel::Fatal, "[Windows] Failed to get user home directory");
            ShowError("[Winows] Failed to find user home directory");
            std::exit(5);
        }

        return std::format("{}/{}/config", user_profile_str, game_name);
    }

    std::string GetSavePath(const char* game_name) {
        std::string config_path = GetConfigPath(game_name);
        return std::format("{}/saves", config_path);
    }
}
