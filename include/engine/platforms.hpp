#pragma once

#include <string>

namespace CE::Platforms {
    std::string GetCachePath(std::string game_name);
    std::string GetConfigPath(std::string game_name);
    std::string GetSavePath(std::string game_name);
    bool SupportsANSI();
    bool EnableANSI();

    void* AlignedAllocate(std::size_t size, std::size_t alignment);
    void AlignedFree(void* ptr) noexcept;
} // namespace CE::Platforms
