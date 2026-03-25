#pragma once

#include "engine/platforms/linux.hpp"

namespace CE::Platforms {
    std::string GetCachePath(const char* game_name);
    std::string GetConfigPath(const char* game_name);
    std::string GetSavePath(const char* game_name);
}
