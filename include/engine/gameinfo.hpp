#pragma once

#include <string>

namespace CE::GameInfo {
    inline std::string gameNameString;
    inline std::string gameVersionString;

    inline int windowHeight;
    inline int windowWidth;
    inline std::string windowTitle;
    inline int maxFPS;

    inline const char* dataFileName = CE_DATA_FILE_NAME;
}