#include "engine/platforms.hpp"

#include <string>

#include "ce_common/platforms.hpp"

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
        return CECommon::Platforms::SupportsANSI();
    }

    bool EnableANSI() {
        return CECommon::Platforms::EnableANSI();
    }

    void* AlignedAllocate(std::size_t size, std::size_t alignment) {
        return Linux::AlignedAllocate(size, alignment);
    }

    void AlignedFree(void* ptr) noexcept {
        Linux::AlignedFree(ptr);
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
        return CECommon::Platforms::SupportsANSI();
    }

    bool EnableANSI() {
        return CECommon::Platforms::EnableANSI();
    }

    void* AlignedAllocate(std::size_t size, std::size_t alignment) {
        return Windows::AlignedAllocate(size, alignment);
    }

    void AlignedFree(void* ptr) noexcept {
        Windows::AlignedFree(ptr);
    }
#endif
} // namespace CE::Platforms