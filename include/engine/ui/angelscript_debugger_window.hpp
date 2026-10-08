#pragma once

#include <optional>

#include "engine/scripting/debugger.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::UI {
    class AngelscriptDebuggerWindow {
        public:
            AngelscriptDebuggerWindow(Scripting::SharedScriptDebuger debugger, Scripting::Runtime& runtime, Common::FS::VFS::VFS& vfs) : mDebugger(debugger), mRuntime(runtime), mVFS(vfs) {
                for (const auto& file : mRuntime.GetScriptSectionNames()) {
                    // very unlikely but best to be sure
                    if (!mVFS.FileExists(file)) {
                        CE_LOG(LogLevel::Error, "[ASDebugWindow] Script file '{}' does not exist", file);
                        mCachedScriptCodeSource.emplace_back("<File not found>");
                        continue;
                    }

                    const auto file_data = mVFS.OpenFile(file);

                    if (!file_data->IsOpen()) {
                        CE_LOG(LogLevel::Error, "[ASDebugWindow] Failed to open script file '{}'", file);
                        mCachedScriptCodeSource.emplace_back("<Could not open file>");
                        continue;
                    }

                    std::optional<std::string> file_string = file_data->ReadToString();
                    if (!file_string) {
                        CE_LOG(LogLevel::Error, "[ASDebugWindow] Failed to get file string '{}'", file);
                        mCachedScriptCodeSource.emplace_back("<Could not read file>");
                        continue;
                    }

                    mCachedScriptCodeSource.push_back(std::move(*file_string));
                }
            }

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
            std::vector<std::string> mCachedScriptCodeSource;
    };
}
