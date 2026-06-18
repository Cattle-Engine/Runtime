#include "engine/engine.hpp"

namespace CE {
    void Engine::ParseProgramArguments(std::vector<std::string> args) {
        for (const std::string& arg : args) {
            size_t split = arg.find('=');

            if (split == std::string::npos) {
                continue;
            }

            std::string key = arg.substr(0, split);
            std::string value = arg.substr(split + 1);

            if (key == "output_debug_as_info") {
                mProgramArgs.OutputDebugASInfo = (value == "true");
            }
        }
    }
}