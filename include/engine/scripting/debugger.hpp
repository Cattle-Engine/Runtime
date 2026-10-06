#pragma once

#include <string>
#include <vector>
#include <angelscript.h>
#include <debugger/debugger.h>

namespace CE::Scripting::Debugger {
    class Debugger : public CDebugger {
    public:
        struct Breakpoint {
            std::string File;
            int Line = 0;
            bool Enabled = true;
        };

        void TakeCommands(asIScriptContext* ctx) override;
        void Output(const std::string& string) override;
        void LineCallback(asIScriptContext* ctx) override;

        bool IsPaused() const {
            return mPaused;
        }

        asIScriptContext* GetContext() const {
            return mContext;
        }

        void Continue();
        void StepInto();
        void StepOver();
        void StepOut();

    private:
        enum class StepMode {
            None,
            Into,
            Over,
            Out
        };

        struct Location {
            std::string File;
            int Line = 0;
        };

        bool ShouldBreak(asIScriptContext* ctx, const Location& location);

        std::string mCurrentFile;
        int mCurrentLine = 0;
        asIScriptContext* mContext = nullptr;
        bool mPaused = false;
        StepMode mStepMode = StepMode::None;
        int mStepStackDepth = 0;
        std::string mConsoleText;
        std::vector<Breakpoint> mBreakpoints;
    };
}