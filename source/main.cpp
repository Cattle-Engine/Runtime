#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"
#include "engine/instance.hpp"


int main(int argc, char *argv[]) {
    // Check for flags
    for (int I = 1; I < argc; I++) {
        std::string arg = argv[I];

    }
    CE::Log(CE::LogLevel::Info, "Cattle Engine");
    CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Core::engineVersionString);

    try {
        CE::Instance instance("data.tcf", true);
        // DO NOT REMOVE THIS, SDL_GPU HAS A BUG IN ITS INTERNAL TEXTURE CACHE THAT
        // HAS A RACE CONDITION THAT CAUSES THE SECOND INSTANCE TO SEGFAULT.
        // I DON'T KNOW WHY THIS TINY DELAY FIXES IT BUT IT JUST DOES. 
        // DO NOT REMOVE
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CE::Instance instance2("data.tcf", true);

        while (!instance.ShouldExit() || !instance2.ShouldExit()) {
            instance.Update();
            instance2.Update();
        }

    } catch (std::runtime_error& e) {
        CE::Log(CE::LogLevel::Fatal, "[Startup] Fatal error: {}", e.what());
    }

    return 0;
}