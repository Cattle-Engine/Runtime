#include <string>

#include "engine/platforms.hpp"

#if defined(__linux__)
    #include "engine/platforms/linux.hpp"
#elif defined(_WIN32)
    #include "engine/platforms/windows.hpp"
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

    #if defined(_WIN32)
        std::string GetCachePath(const char* game_name) {
            return CE::Platforms::Windows::GetCachePath(game_name);
        }

        std::string GetConfigPath(const char* game_name) {
            return CE::Platforms::Windows::GetConfigPath(game_name);
        }

        std::string GetSavePath(const char* game_name) {
            return CE::Platforms::Windows::GetSavePath(game_name);
        }
    #endif
}