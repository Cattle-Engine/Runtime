#include "engine/input/mouse.hpp"
#include "engine/common/sdl_events.hpp"
#include <algorithm>

namespace CE::Input {

    Mouse::Mouse(int windowID)
        : gWindowID(windowID) {}

    constexpr int Mouse::ToIndex(MouseButtons button) {
        switch (button) {
            case MouseButtons::LEFT:   return 0;
            case MouseButtons::MIDDLE: return 1;
            case MouseButtons::RIGHT:  return 2;
            case MouseButtons::X1:     return 3;
            case MouseButtons::X2:     return 4;
            default: return -1;
        }
    }

	    void Mouse::Update() {
	        gPrevious = gCurrent;

	        gDX = gDY = 0;
	        gWheelX = gWheelY = 0;

	        float x = 0.0f, y = 0.0f;
	        SDL_GetMouseState(&x, &y);
	        gX = static_cast<int>(x);
	        gY = static_cast<int>(y);

	        auto indices = CE::SDL_Events::GetWindowEventIndices(gWindowID);

        for (size_t idx : indices) {
            const SDL_Event& event = CE::SDL_Events::gEvents[idx];

            switch (event.type) {

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    auto btn = static_cast<MouseButtons>(event.button.button);
                    int i = ToIndex(btn);

                    if (i >= 0) {
                        gCurrent[i] = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_MOTION: {
                    gX = event.motion.x;
                    gY = event.motion.y;

                    gDX += event.motion.xrel;
                    gDY += event.motion.yrel;
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    gWheelX += event.wheel.x;
                    gWheelY += event.wheel.y;
                    break;
                }
            }
        }
    }

    bool Mouse::IsButtonDown(MouseButtons button) const {
        int i = ToIndex(button);
        return (i >= 0) ? gCurrent[i] : false;
    }

    bool Mouse::IsButtonPressed(MouseButtons button) const {
        int i = ToIndex(button);
        return (i >= 0) ? (gCurrent[i] && !gPrevious[i]) : false;
    }

    bool Mouse::IsButtonReleased(MouseButtons button) const {
        int i = ToIndex(button);
        return (i >= 0) ? (!gCurrent[i] && gPrevious[i]) : false;
    }

}
