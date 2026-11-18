#pragma once

#include <string>

// TODO: Find out if anything needs this...
#ifndef CE_DATA_FILE_NAME
#define CE_DATA_FILE_NAME "data.tcf"
#endif

#include "./window_mode.hpp"

namespace CE {
    struct GameInfo {
        std::string game_name;
        std::string game_version;

        std::string game_package_id;
        
        int windowHeight;
        int windowWidth;
        std::string windowTitle;
        int maxFPS;
        bool enableVSync;
        std::string rendererName;
        bool resizableWindow;
        std::string windowIcon;

        std::string startupFileName;

        std::string dataFileName;

        int minWindowWidth;
        int minWindowHeight;

        int maxWindowWidth;
        int maxWindowHeight;

        bool pauseRenderingWhenFocusLostInWindowedMode;
        bool pauseUpdateWhenFocusLost;

        Common::WindowMode windowMode;
    };
} // namespace CE