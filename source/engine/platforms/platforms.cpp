#include <string>

#include "engine/platforms.hpp"

#if defined(__linux__)
    #include "engine/platforms/linux.hpp"
#else 
    #error "Unsupported platform"
#endif

namespace CE::Platforms {
    #if defined(__linux__)
        std::string GetCachePath(const char* game_name) {
            return CE::Platforms::Linux::GetCachePath(game_name);
        }

        std::string GetConfigPath(const char* game_name) {
            return CE::Platforms::Linux::GetConfigPath(game_name);
        }

        std::string GetSavePath(const char* game_name) {
            return CE::Platforms::Linux::GetSavePath(game_name);
        }
    #endif
}