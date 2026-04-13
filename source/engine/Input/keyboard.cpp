#include "engine/input/keyboard.hpp"

namespace CE::Input {
    Keyboard::Keyboard(int windowID) {
        gWindowID = windowID;
    }

    void Keyboard::Update() {
        memcpy(gPrevious, gCurrent, sizeof(gCurrent));

        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_KEY_DOWN ||
                event.type == SDL_EVENT_KEY_UP) {

                SDL_Scancode sc = event.key.scancode;

                gCurrent[sc] = (event.type == SDL_EVENT_KEY_DOWN);
            }
        }
    }

    bool Keyboard::IsKeyPressed(KeyboardKeys key) const {

    }

    bool Keyboard::IsKeyDown(KeyboardKeys key) const {
        
    }
}