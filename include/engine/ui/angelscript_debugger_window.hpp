#pragma once

#include "engine/scripting/debugger.hpp"

namespace CE::UI {
    class AngelscriptDebuggerWindow {
        public:
            AngelscriptDebuggerWindow(Scripting::SharedScriptDebuger debugger) : mDebugger(debugger) {}
            void DrawWindow();

            // the window has a close button, this will reshow the window if the user has clicked close
            void OpenWindow();
        private:
            void DrawBreakPointsTab();

            bool mWindowOpen = true;
            Scripting::SharedScriptDebuger mDebugger;
    };
}