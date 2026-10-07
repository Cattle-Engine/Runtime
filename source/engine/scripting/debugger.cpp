#include "engine/scripting/debugger.hpp"

#include <algorithm>
#include <sstream>
#include <format>

#include "debugger/debugger.h"
#include <angelscript.h>

namespace CE::Scripting {
    void ScriptDebugger::TakeCommands(asIScriptContext* ctx) {
        mContext = ctx;
        mPaused = false;
    }

    void ScriptDebugger::Output(const std::string& string) {
        mConsoleText += string + '\n';
    }

    void ScriptDebugger::LineCallback(asIScriptContext* ctx) {
        const char* section = nullptr;
        const int line = ctx->GetLineNumber(0, nullptr, &section);

        if (section == nullptr || line <= 0) {
            return;
        }

        // By default we ignore callbacks when the context is not active.
        // An application might override this to for example disconnect the
        // debugger as the execution finished.
        if (ctx->GetState() != asEXECUTION_ACTIVE)
            return;

        if (m_action == CONTINUE) {
            if (!CheckBreakPoint(ctx))
                return;
        } else if (m_action == STEP_OVER) {
            if (ctx->GetCallstackSize() > m_lastCommandAtStackLevel) {
                if (!CheckBreakPoint(ctx))
                    return;
            }
        } else if (m_action == STEP_OUT) {
            if (ctx->GetCallstackSize() >= m_lastCommandAtStackLevel) {
                if (!CheckBreakPoint(ctx))
                    return;
            }
        } else if (m_action == STEP_INTO) {
            CheckBreakPoint(ctx);
            // Always break, but we call the check break point anyway
            // to tell user when break point has been reached
        }

        std::stringstream s;
        const char* file = 0;
        int lineNbr = ctx->GetLineNumber(0, 0, &file);
        s << (file ? file : "{unnamed}") << ":" << lineNbr << "; " << ctx->GetFunction()->GetDeclaration() << std::endl;
        Output(s.str());

        TakeCommands(ctx);
        ctx->Suspend();
    }

    void ScriptDebugger::AddFileBreakPoint(const std::string& file, int lineNbr) {
        Output("Setting break point in file '" + file + "' at line " + std::to_string(lineNbr));
        m_breakPoints.emplace_back(file, lineNbr, false);
    }

    bool ScriptDebugger::CheckBreakPoint(asIScriptContext* ctx) {
        if (ctx == nullptr)
            return false;

        const char* section = nullptr;
        const int line = ctx->GetLineNumber(0, nullptr, &section);

        if (section == nullptr || line <= 0)
            return false;

        const std::string file = section;
        asIScriptFunction* func = ctx->GetFunction();

        if (func == nullptr) {
            return false;
        }

        if (m_lastFunction != func) {
            for (size_t i = 0; i < m_breakPoints.size(); ++i) {
                auto& bp = m_breakPoints[i];

                if (bp.func) {
                    if (bp.name == func->GetName()) {
                        std::stringstream s;
                        s << "Entering function '" << bp.name << "'. Transforming it into break point\n";
                        Output(s.str());

                        bp.name = file;
                        bp.lineNbr = line;
                        bp.func = false;
                        bp.needsAdjusting = false;
                    }
                } else if (bp.needsAdjusting && bp.name == file) {
                    const int adjustedLine = func->FindNextLineWithCode(bp.lineNbr);

                    if (adjustedLine >= 0) {
                        bp.needsAdjusting = false;

                        if (adjustedLine != bp.lineNbr) {
                            std::stringstream s;
                            s << "Moving break point " << i << " in file '" << file
                              << "' to next line with code at line " << adjustedLine << '\n';
                            Output(s.str());

                            bp.lineNbr = adjustedLine;
                        }
                    }
                }
            }

            m_lastFunction = func;
        }

        for (size_t i = 0; i < m_breakPoints.size(); ++i) {
            const auto& bp = m_breakPoints[i];

            if (!bp.func && bp.lineNbr == line && bp.name == file) {
                std::stringstream s;
                s << "Reached break point " << i << " in file '" << file << "' at line " << line << '\n';
                Output(s.str());

                return true;
            }
        }

        return false;
    }

    void ScriptDebugger::Continue() {
        m_action = CONTINUE;
    }

    void ScriptDebugger::StepInto() {
        m_action = STEP_INTO;
    }

    void ScriptDebugger::StepOver() {
        m_action = STEP_OVER;
    }

    void ScriptDebugger::StepOut() {
        m_action = STEP_OUT;
    }

    std::vector<ScriptDebugger::BreakPointInfo> ScriptDebugger::GetBreakPoints() const {
        std::vector<ScriptDebugger::BreakPointInfo> info;

        info.reserve(m_breakPoints.size());
        for (const BreakPoint& breakpoint : m_breakPoints) {
            info.push_back({.name=breakpoint.name, .line=breakpoint.lineNbr, .function=breakpoint.func});
        }

        return info;
    }

    bool ScriptDebugger::RemoveFuncBreakPoint(const std::string& func) {
        for (size_t i = 0; i < m_breakPoints.size(); i++) {
            if (m_breakPoints[i].name == func) {
                if (m_breakPoints[i].func) {
                    m_breakPoints.erase(m_breakPoints.begin() + i);
                    Output("Removed function breakpoint: " + func);
                    return true;
                }
                Output(std::format("'{}' is not a function", func));
                return false;
            }
        }
        Output(std::format("Function '{}' could not be found", func));
        return false;
    }

    bool ScriptDebugger::RemoveFileBreakPoint(const std::string& file, int line) {
        for (size_t i = 0; i < m_breakPoints.size(); i++) {
            if (m_breakPoints[i].name == file && m_breakPoints[i].lineNbr == line && !m_breakPoints[i].func) {
                m_breakPoints.erase(m_breakPoints.begin() + i);
                Output(std::format("Removed breakpoint for file '{}', at line {}", file, line));
                return true;
            }
        }

        Output(std::format("Could not find breakpoint for file '{}', at line {}", file, line));
        return false;
    }

    std::string ScriptDebugger::GetDisplayFunctionName(asIScriptFunction *func) const {
        if (func == nullptr) {
            return {};
        }

        const std::string internal_name = func->GetName();
        const std::string declaration = func->GetDeclaration();

        if (!internal_name.empty()) {
            if (const auto it = mGeneratedSymbolLookup.find(internal_name); it != mGeneratedSymbolLookup.end()) {
                return it->second;
            }

            if (const auto it = mGeneratedSymbolQualifiedLookup.find(internal_name); it != mGeneratedSymbolQualifiedLookup.end()) {
                return it->second;
            }
        }

        if (!declaration.empty()) {
            for (const auto& [key, display_name] : mGeneratedSymbolLookup) {
                if (declaration.find(key) != std::string::npos) {
                    return display_name;
                }
            }
        }

        for (const auto& symbol : mSymbolmap) {
            if (symbol.InternalName == internal_name || symbol.QualifiedName == internal_name ||
                symbol.DisplayName == internal_name) {
                return symbol.DisplayName;
            }

            if (!declaration.empty() && (declaration.find(symbol.InternalName) != std::string::npos ||
                                         declaration.find(symbol.QualifiedName) != std::string::npos ||
                                         declaration.find(symbol.DisplayName) != std::string::npos)) {
                return symbol.DisplayName;
            }
        }

        if (!internal_name.empty()) {
            return internal_name;
        }

        return declaration;
    }

    void ScriptDebugger::Attach(asIScriptContext* ctx) {
        if (ctx == nullptr)
            return;

        using LineCallback = void (*)(asIScriptContext*, void*);

        LineCallback callback = [](asIScriptContext* ctx, void* param) {
            auto* debugger = static_cast<ScriptDebugger*>(param);
            debugger->LineCallback(ctx);
        };

        ctx->SetLineCallback(
            asFunctionPtr(callback),
            this,
            asCALL_CDECL
        );

        Output(std::format("Attached to script context {:p}", static_cast<void*>(ctx)));
    }

    std::vector<ScriptDebugger::CallstackEntry> ScriptDebugger::GetCallStack() {
        std::vector<CallstackEntry> result;

        if (!mContext) {
            return result;
        }

        for (asUINT n = 0; n < mContext->GetCallstackSize(); n++) {
            const char* file = nullptr;
            int line = mContext->GetLineNumber(n, 0, &file);

            CallstackEntry entry;
            entry.file = file ? file : "{unnamed}";
            entry.line = line;
            entry.function = GetDisplayFunctionName(mContext->GetFunction(n));

            result.push_back(std::move(entry));
        }

        return result;
    }

    ScriptDebugger::GCStatistics ScriptDebugger::GetGCStats() {
        GCStatistics s;
        if (mContext == nullptr) {
            Output("No script running");
            return {};
        }

        asIScriptEngine* engine = mContext->GetEngine();

        engine->GetGCStatistics(&s.CurrentSize, &s.TotalDestructions, &s.TotalDetected, &s.NewObjects, &s.TotalNewDestructions);
        return s;
    }
} // namespace CE::Scripting