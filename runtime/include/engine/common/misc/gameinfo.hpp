#pragma once

#include <string>

// TODO: Find out if anything needs this...
#ifndef CE_DATA_FILE_NAME
#define CE_DATA_FILE_NAME "data.tcf"
#endif

#include "engine/common/window.hpp"

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

        Common::Window::WindowMode windowMode;
    };
} // namespace CE