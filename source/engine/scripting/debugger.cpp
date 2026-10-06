#include "engine/scripting/debugger.hpp"
#include <angelscript.h>

namespace CE::Scripting::Debugger {
    void Debugger::TakeCommands(asIScriptContext* ctx) {
        mContext = ctx;
        mPaused = false;
    }

    void Debugger::Output(const std::string& string) {
        mConsoleText += string;
    }
    
    void Debugger::LineCallback(asIScriptContext* ctx) {
        const char* section = nullptr;
        const int line = ctx->GetLineNumber(0, nullptr, &section);

        if (section == nullptr || line <= 0) {
            return;
        }

        mCurrentFile = section;
        mCurrentLine = line;

        const Location location{
            .File = section,
            .Line = line
        };

        if (!ShouldBreak(ctx, location)) {
            return;
        }

        mContext = ctx;
        mPaused = true;

        ctx->Suspend();
    }

    bool Debugger::ShouldBreak(asIScriptContext* ctx, const Location& location) {
        for (const Breakpoint& breakpoint : mBreakpoints) {
            if (!breakpoint.Enabled) {
                continue;
            }

            if (breakpoint.File == location.File &&
                breakpoint.Line == location.Line) {
                return true;
            }
        }
    }
}