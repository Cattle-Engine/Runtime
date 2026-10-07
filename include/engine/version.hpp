#pragma once

#include <format>
#include <string>

#include <git_version.hpp>

namespace CE::Version {
    inline constexpr const char* engineVersionString = "Alpha 0.2";
    inline constexpr int engineVersionMajor = 0;
    inline constexpr int engineVersionMinor = 2;
    inline constexpr int engineVersionPatch = 0;

    inline constexpr bool IsGitDirty() {
        return std::string_view{CE_GIT_ISDIRTY} == "true";
    }

    #ifdef CE_DEBUG
        inline constexpr std::string_view CE_BUILD_CONFIG = "Debug";
    #else
        inline constexpr std::string_view CE_BUILD_CONFIG = "Release";
    #endif

    inline std::string GetBuildString() {
        std::string dirty = IsGitDirty() ? ", Dirty" : "";

        if (std::string_view{CE_GIT_TAGS} != "unknown" && !std::string_view{CE_GIT_TAGS}.empty()) {
            return std::format(
                "{}-({}, {}, {}, {}{})",
                engineVersionString,
                CE_GIT_TAGS,
                CE_GIT_HASH,
                CE_GIT_BRANCH,
                CE_BUILD_CONFIG,
                dirty
            );
        }

        return std::format(
            "{}-({}, {}, {}{})",
            engineVersionString,
            CE_GIT_HASH,
            CE_GIT_BRANCH,
            CE_BUILD_CONFIG,
            dirty
        );
    }
} // namespace CE::Version