#pragma once

#include <string>

#ifndef CE_DATA_FILE_NAME
    #define CE_DATA_FILE_NAME "data.tcf"
#endif

namespace CE::GameInfo {
    inline std::string gameNameString;
    inline std::string gameVersionString;

    inline int windowHeight;
    inline int windowWidth;
    inline std::string windowTitle;
    inline int maxFPS;
    inline bool enableVSync;

    inline const char* dataFileName = CE_DATA_FILE_NAME;
}
