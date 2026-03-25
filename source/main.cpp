#include <string>
#include <iostream>

#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"

int main(int argc, char *argv[]) {
    // Check for flags
    for (int I = 1; I < argc; I++) {
        std::string arg = argv[I];

    }
    CE::Log(CE::LogLevel::Info, "Cattle Engine");
    CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Core::engineVersionString);
    return 0;
}