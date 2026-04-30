#pragma once

#include <string>

namespace CE::Platforms::Linux {
    std::string GetCachePath(const char* game_name);
    std::string GetConfigPath(const char* game_name);
    std::string GetSavePath(const char* game_name);
    bool SupportsANSI();
    bool EnableANSI();
}