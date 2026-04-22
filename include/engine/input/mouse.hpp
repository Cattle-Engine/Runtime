#pragma once

#include <array>
#include <cstdint>
#include <SDL3/SDL.h>

namespace CE::Input {

    enum class MouseButtons {
        LEFT   = SDL_BUTTON_LEFT,
        MIDDLE = SDL_BUTTON_MIDDLE,
        RIGHT  = SDL_BUTTON_RIGHT,
        X1     = SDL_BUTTON_X1,
        X2     = SDL_BUTTON_X2
    };

    class Mouse {
    public:
        Mouse(int windowID);

        void Update();

        bool IsButtonDown(MouseButtons button) const;
        bool IsButtonPressed(MouseButtons button) const;
        bool IsButtonReleased(MouseButtons button) const;

        int GetX() const { return gX; }
        int GetY() const { return gY; }

        int GetDeltaX() const { return gDX; }
        int GetDeltaY() const { return gDY; }

        int GetWheelX() const { return gWheelX; }
        int GetWheelY() const { return gWheelY; }

    private:
        static constexpr int ToIndex(MouseButtons button);

    private:
        int gWindowID;

        std::array<bool, 5> gCurrent{};
        std::array<bool, 5> gPrevious{};

        int gX = 0, gY = 0;
        int gDX = 0, gDY = 0;

        int gWheelX = 0, gWheelY = 0;
    };

}