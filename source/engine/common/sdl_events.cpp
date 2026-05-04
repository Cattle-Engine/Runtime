#include <SDL3/SDL.h>

#include "engine/common/sdl_events.hpp"

namespace CE::SDL_Events {
    std::vector<SDL_Event> CE::SDL_Events::gEvents;
    std::unordered_map<SDL_WindowID, std::vector<size_t>> CE::SDL_Events::gWindowEventIndices;

    void Update() {
        gEvents.clear();
        gWindowEventIndices.clear();

        SDL_Event e;

        while (SDL_PollEvent(&e)) {

            SDL_WindowID windowID = 0;

            switch (e.type) {

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    windowID = e.key.windowID;
                    break;

                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_TEXT_EDITING:
                    windowID = e.text.windowID;
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    windowID = e.motion.windowID;
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    windowID = e.button.windowID;
                    break;

                case SDL_EVENT_MOUSE_WHEEL:
                    windowID = e.wheel.windowID;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    windowID = e.window.windowID;
                    break;
                    
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    windowID = e.window.windowID;
                    break;
                    
                case SDL_EVENT_QUIT:
                    windowID = 0;
                    break;

                default:
                    windowID = 0;
                    break;
            }

            size_t index = gEvents.size();
            gEvents.push_back(e);

            if (windowID != 0)
                gWindowEventIndices[windowID].push_back(index);
        }
    }

    std::span<const size_t> GetWindowEventIndices(int windowID) {
        static const std::vector<size_t> empty;

        auto it = gWindowEventIndices.find(windowID);
        if (it == gWindowEventIndices.end())
            return {};

        return it->second;
    }
}
