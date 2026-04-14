#include <string>
#include <iostream>

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
        CE::Instance instance2("data.tcf", true);

        while (!instance.ShouldExit() && !instance2.ShouldExit()) {
            instance.Update();
            instance2.Update();
        }

    } catch (std::runtime_error& e) {
        CE::Log(CE::LogLevel::Fatal, "[Startup] Fatal error: {}", e.what());
    }

    return 0;
}