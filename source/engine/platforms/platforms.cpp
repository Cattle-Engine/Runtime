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
        std::string GetCachePath(std::string game_name) {
            return CE::Platforms::Linux::GetCachePath(game_name.c_str());
        }

        std::string GetConfigPath(std::string game_name) {
            return CE::Platforms::Linux::GetConfigPath(game_name.c_str());
        }

        std::string GetSavePath(std::string game_name) {
            return CE::Platforms::Linux::GetSavePath(game_name.c_str());
        }

        bool SupportsANSI() {
            return Linux::SupportsANSI();
        }

        bool EnableANSI() {
            return Linux::EnableANSI();
        }
    #endif

    #if defined(_WIN32)
        std::string GetCachePath(std::string game_name) {
            return CE::Platforms::Windows::GetCachePath(game_name.c_str());
        }

        std::string GetConfigPath(std::string game_name) {
            return CE::Platforms::Windows::GetConfigPath(game_name.c_str());
        }

        std::string GetSavePath(std::string game_name) {
            return CE::Platforms::Windows::GetSavePath(game_name.c_str());
        }

        bool SupportsANSI() {
            return Windows::SupportsANSI();
        }

        bool EnableANSI() {
            return Windows::EnableANSI();
        }
    #endif
}