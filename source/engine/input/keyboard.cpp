#include "engine/input/keyboard.hpp"
#include "engine/common/events.hpp"

namespace CE::Input {
    Keyboard::Keyboard(int windowID) {
        gWindowID = windowID;
    }

    void Keyboard::Update() {
        memcpy(gPrevious, gCurrent, sizeof(gCurrent));

        auto indices = CE::Events::GetWindowEventIndices(gWindowID);

        for (size_t idx : indices) {
            const SDL_Event& event = CE::Events::gEvents[idx];

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

	    // Used for debuging, gives a string of all keys pressed for a frame eg awk
	    std::string Keyboard::GetPressedKeysString() const {
	        std::string out;

	        for (int sc = 0; sc < SDL_SCANCODE_COUNT; sc++) {
	            if (!gCurrent[sc])
	                continue;

            SDL_Scancode key = (SDL_Scancode)sc;

            bool shift =
                gCurrent[SDL_SCANCODE_LSHIFT] ||
                gCurrent[SDL_SCANCODE_RSHIFT];

            char c = 0;

            switch (key) {
                case SDL_SCANCODE_A: c = shift ? 'A' : 'a'; break;
                case SDL_SCANCODE_B: c = shift ? 'B' : 'b'; break;
                case SDL_SCANCODE_C: c = shift ? 'C' : 'c'; break;
                case SDL_SCANCODE_D: c = shift ? 'D' : 'd'; break;
                case SDL_SCANCODE_E: c = shift ? 'E' : 'e'; break;
                case SDL_SCANCODE_F: c = shift ? 'F' : 'f'; break;
                case SDL_SCANCODE_G: c = shift ? 'G' : 'g'; break;
                case SDL_SCANCODE_H: c = shift ? 'H' : 'h'; break;
                case SDL_SCANCODE_I: c = shift ? 'I' : 'i'; break;
                case SDL_SCANCODE_J: c = shift ? 'J' : 'j'; break;
                case SDL_SCANCODE_K: c = shift ? 'K' : 'k'; break;
                case SDL_SCANCODE_L: c = shift ? 'L' : 'l'; break;
                case SDL_SCANCODE_M: c = shift ? 'M' : 'm'; break;
                case SDL_SCANCODE_N: c = shift ? 'N' : 'n'; break;
                case SDL_SCANCODE_O: c = shift ? 'O' : 'o'; break;
                case SDL_SCANCODE_P: c = shift ? 'P' : 'p'; break;
                case SDL_SCANCODE_Q: c = shift ? 'Q' : 'q'; break;
                case SDL_SCANCODE_R: c = shift ? 'R' : 'r'; break;
                case SDL_SCANCODE_S: c = shift ? 'S' : 's'; break;
                case SDL_SCANCODE_T: c = shift ? 'T' : 't'; break;
                case SDL_SCANCODE_U: c = shift ? 'U' : 'u'; break;
                case SDL_SCANCODE_V: c = shift ? 'V' : 'v'; break;
                case SDL_SCANCODE_W: c = shift ? 'W' : 'w'; break;
                case SDL_SCANCODE_X: c = shift ? 'X' : 'x'; break;
                case SDL_SCANCODE_Y: c = shift ? 'Y' : 'y'; break;
                case SDL_SCANCODE_Z: c = shift ? 'Z' : 'z'; break;

                case SDL_SCANCODE_SPACE: c = ' '; break;
                case SDL_SCANCODE_RETURN: c = '\n'; break;
                case SDL_SCANCODE_TAB: c = '\t'; break;

                default:
                    break;
            }

            if (c) {
                if (!out.empty())
                    out += ' ';

                out += c;
            }
        }

        return out;
    }
}
