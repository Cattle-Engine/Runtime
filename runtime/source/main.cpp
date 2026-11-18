#include <format>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "ce_runtime/common/misc/arguments.hpp"
#include "ce_runtime/common/misc/error_box.hpp"
#include "ce_runtime/common/tracelog.hpp"
#include "ce_runtime/engine.hpp"
#include "ce_runtime/memory/allocator.hpp"

int main(int argc, char* argv[]) {
    // get the executable base path to combine with the data file name (data.tcf)
    const char* base = SDL_GetBasePath();
    if (!base) {
        CE_LOG(CE::LogLevel::Fatal, "[Main] SDL_GetBasePath returned nullptr");
        return 1;
    }

    // turn argc and argv into a std::vector so the argument parser can use them
    std::vector<std::string> args(argv + 1, argv + argc);

    // parse the arguments
    CE::EngineArguements engine_args;
    CE::Common::ParseProgramArguments(args, engine_args);

    // create the engine and instance
    try {
        CE::Memory::EnableTracking(true);
        CE::Engine engine(std::format("{}data.tcf", base), true, engine_args);
        if (!engine.CreateInstance("main", true))
            return 1;
        return engine.Run();
    } catch (std::runtime_error& e) {
        ShowError(e.what());
        return 1;
    }
}
