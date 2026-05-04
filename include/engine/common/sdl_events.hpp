#pragma once

#include <vector>
#include <span>
#include <unordered_map>
#include <SDL3/SDL.h>

// Due to the lovely way SDL handles events I'm having to create
// an event manager...

namespace CE::SDL_Events {
    extern std::vector<SDL_Event> gEvents;
    extern std::unordered_map<SDL_WindowID, std::vector<size_t>> gWindowEventIndices;
    void Update();
    std::span<const size_t> GetWindowEventIndices(int windowID);
}