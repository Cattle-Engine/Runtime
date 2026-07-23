#include "engine/common/utils/is_fullscreen.hpp"

namespace CE::Utils {
    bool IsWindowFullScreen(SDL_Window* window) {
        auto flags = SDL_GetWindowFlags(window);

        if (flags & SDL_WINDOW_FULLSCREEN) {
            return true;
        }

        return false;
    }
} // namespace CE::Utils