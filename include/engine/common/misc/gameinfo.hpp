#pragma once

#include <string>

#ifndef CE_DATA_FILE_NAME
    #define CE_DATA_FILE_NAME "data.tcf"
#endif

namespace CE {
    struct GameInfo {
        std::string gameNameString;
        std::string gameVersionString;

        int windowHeight;
        int windowWidth;
        std::string windowTitle;
        int maxFPS;
        bool enableVSync;
        std::string rendererName;
        bool fullscreen;
        bool resizableWindow;
        std::string windowIcon;

        std::string startupFileName;
        const char* dataFileName;
    };
}