#pragma once

#include <string>

namespace CE::Platforms {
    std::string GetCachePath(std::string game_name);
    std::string GetConfigPath(std::string game_name);
    std::string GetSavePath(std::string game_name);
    bool SupportsANSI();
    bool EnableANSI(); 
}
