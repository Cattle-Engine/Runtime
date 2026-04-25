#include "engine/common/fullscreen.hpp"

#include "engine/common/tracelog.hpp"

namespace CE {
    bool ApplyFullscreenMode(SDL_Window* window, int width, int height) {
        if (window == nullptr) {
            CE::Log(CE::LogLevel::Error, "[Window] Cannot apply fullscreen mode to a null window");
            return false;
        }

        const SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
        if (displayID == 0) {
            CE::Log(CE::LogLevel::Warn,
                "[Window] Failed to find display for window, using desktop fullscreen: {}",
                SDL_GetError());
            return SDL_SetWindowFullscreenMode(window, nullptr) && SDL_SetWindowFullscreen(window, true);
        }

        const SDL_DisplayMode* desktopMode = SDL_GetDesktopDisplayMode(displayID);

        SDL_DisplayMode fullscreenMode = {};
        const float refreshRate = desktopMode != nullptr ? desktopMode->refresh_rate : 0.0f;
        const bool foundMode = SDL_GetClosestFullscreenDisplayMode(
            displayID,
            width,
            height,
            refreshRate,
            false,
            &fullscreenMode
        );

        if (!foundMode) {
            CE::Log(CE::LogLevel::Warn,
                "[Window] No matching exclusive fullscreen mode for {}x{}, using desktop fullscreen: {}",
                width,
                height,
                SDL_GetError());
            return SDL_SetWindowFullscreenMode(window, nullptr) && SDL_SetWindowFullscreen(window, true);
        }

        if (!SDL_SetWindowFullscreenMode(window, &fullscreenMode)) {
            CE::Log(CE::LogLevel::Warn,
                "[Window] Failed to set fullscreen mode {}x{} @ {} Hz, using desktop fullscreen: {}",
                fullscreenMode.w,
                fullscreenMode.h,
                fullscreenMode.refresh_rate,
                SDL_GetError());
            return SDL_SetWindowFullscreenMode(window, nullptr) && SDL_SetWindowFullscreen(window, true);
        }

        if (!SDL_SetWindowFullscreen(window, true)) {
            CE::Log(CE::LogLevel::Error,
                "[Window] Failed to enter fullscreen after selecting {}x{} @ {} Hz: {}",
                fullscreenMode.w,
                fullscreenMode.h,
                fullscreenMode.refresh_rate,
                SDL_GetError());
            return false;
        }

        CE::Log(CE::LogLevel::Info,
            "[Window] Fullscreen mode set to {}x{} @ {} Hz",
            fullscreenMode.w,
            fullscreenMode.h,
            fullscreenMode.refresh_rate);
        return true;
    }
}
