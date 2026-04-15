#include "engine/input/keyboard.hpp"

namespace CE::Input {
    Keyboard::Keyboard(int windowID) {
        gWindowID = windowID;
    }

    void Keyboard::Update() {
        memcpy(gPrevious, gCurrent, sizeof(gCurrent));

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            // If the event doesn't belong to the window ID skip
            if (event.type >= SDL_EVENT_WINDOW_FIRST &&
                event.type <= SDL_EVENT_WINDOW_LAST) {

                if (event.window.windowID != gWindowID) {
                    continue;
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN ||
                event.type == SDL_EVENT_KEY_UP) {

                SDL_Scancode sc = event.key.scancode;

                gCurrent[sc] = (event.type == SDL_EVENT_KEY_DOWN);
            }
        }
    }

    bool Keyboard::IsKeyPressed(KeyboardKeys key) const {
        SDL_Scancode sc = static_cast<SDL_Scancode>(key);
        return gCurrent[sc] && !gPrevious[sc];
    }

    bool Keyboard::IsKeyReleased(KeyboardKeys key) const {
        SDL_Scancode sc = static_cast<SDL_Scancode>(key);
        return !gCurrent[sc] && gPrevious[sc];
    }

    bool Keyboard::IsKeyDown(KeyboardKeys key) const {
        return gCurrent[static_cast<SDL_Scancode>(key)];
    }
}