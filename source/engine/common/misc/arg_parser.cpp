#include "engine/common/misc/arguments.hpp"

namespace CE::Common {
    void ParseProgramArguments(std::vector<std::string> args, EngineArguements& output) {
        for (const std::string& arg : args) {
            size_t split = arg.find('=');

            if (split == std::string::npos) {
                continue;
            }

            std::string key = arg.substr(0, split);
            std::string value = arg.substr(split + 1);

            if (key == "output_debug_as_info") {
                output.OutputDebugASInfo = (value == "true");
            } else if (key == "output_debug_as_info_path") {
                // Must be an absloute path to the directory
                output.OutputDebugASInfoPath = value;
            }
        }
    }
} // namespace CE::Common