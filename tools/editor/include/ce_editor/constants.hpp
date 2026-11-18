#pragma once

#include "ce_runtime/common/misc/window_mode.hpp"

namespace CE::Editor::Constants {
    constexpr const char* kWindowTitle = "CE Editor";
    constexpr const char* kWindowIconPath = "/textures/window_icon.png";
    constexpr int kDefaultWindowHeight = 900;
    constexpr int kDefaultWindowWidth = 1600;
    constexpr Common::WindowMode kDefaultWindowMode = Common::WindowMode::Windowed;
    constexpr int kMaxFPS = 60;

    constexpr bool kWindowResizable = true; // basically never to be changed lol
}