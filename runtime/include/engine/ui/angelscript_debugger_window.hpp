#pragma once

#include <optional>

#include "engine/scripting/debugger.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::UI {
    class AngelscriptDebuggerWindow {
        public:
            AngelscriptDebuggerWindow(Scripting::SharedScriptDebuger debugger, Scripting::Runtime& runtime, Common::FS::VFS::VFS& vfs) : mDebugger(debugger), mRuntime(runtime), mVFS(vfs) {}

            void DrawWindow();

            // the window has a close button, this will reshow the window if the user has clicked close
            void OpenWindow();
        private: 
            void DrawStatisticsTab();
            void DrawCallStackTab();
            void DrawBreakPointsTab();
            void DrawDebuggerOutputTab();
            void DrawLoadedScriptsTab();
            void DrawLocalVariablesTab();
            void DrawGlobalVariablesTab();
            void DrawMemberPropertiesTab();
            void DrawContexts();
            std::string mLastDebuggerFile;
            bool mWasPaused = false;
            std::optional<Scripting::ScriptDebugger::BreakPointInfo> mSelectedBreakpoint;
            bool mWindowOpen = false;
            Scripting::SharedScriptDebuger mDebugger;
            Scripting::Runtime& mRuntime;
            Common::FS::VFS::VFS mVFS;
    };
}
