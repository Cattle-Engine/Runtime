#pragma once

#include <string>
#include <vector>

namespace CE {
    struct EngineArguements {
        bool OutputDebugASInfo = false;
        std::string OutputDebugASInfoPath = "";
    };

    namespace Common {
        /**
         * @brief A small utlity function to auto parser arguemnts for desktop platforms
         *
         * @param args A std::vector with std::strings for the program args
         * @param output The output EngineArguemnts
         */
        void ParseProgramArguments(std::vector<std::string> args, EngineArguements& output);
    } // namespace Common
} // namespace CE