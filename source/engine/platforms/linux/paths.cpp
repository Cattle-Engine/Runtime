#include <cstdlib>
#include <string>
#include <format>

#include "engine/common/error_box.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/platforms/linux.hpp"

namespace CE::Platforms::Linux {
    std::string GetCachePath(const char* game_name) {
        const char* home_str = std::getenv("HOME");

        if (!home_str) {
            CE::Log(CE::Fatal, "[Linux] Couldn't find user home directory");
            ShowError("[Linux] Unable to find user home directory");
            std::exit(5);
        }

        std::string base_cache = std::format("{}/.cache/{}", home_str , game_name);
        return base_cache;
    }

    std::string GetConfigPath(const char* game_name) {
        const char* home_str = std::getenv("HOME");

        if (!home_str) {
            CE::Log(CE::Fatal, "[Linux] Couldn't find user home directory");
            ShowError("[Linux] Unable to find user home directory");
            std::exit(5);
        }

        std::string base_config = std::format("{}/.config/{}", home_str, game_name);
        return base_config;
    }

    std::string GetSavePath(const char* game_name) {
        std::string config_path = GetConfigPath(game_name);
        return std::format("{}/saves", config_path);
    }
}