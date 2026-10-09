#include "engine/scripting/debugger.hpp"

#include <algorithm>
#include <sstream>
#include <format>
#include <cstring>

#include "debugger/debugger.h"
#include <angelscript.h>

namespace CE::Scripting {
    ScriptDebugger::ContextState* ScriptDebugger::FindContext(asIScriptContext* ctx) {
        const auto it = std::find_if(mContexts.begin(), mContexts.end(), [ctx](const ContextState& state) {
            return state.context == ctx;
        });
        return it == mContexts.end() ? nullptr : &*it;
    }

    const ScriptDebugger::ContextState* ScriptDebugger::FindContext(asIScriptContext* ctx) const {
        const auto it = std::find_if(mContexts.begin(), mContexts.end(), [ctx](const ContextState& state) {
            return state.context == ctx;
        });
        return it == mContexts.end() ? nullptr : &*it;
    }

    bool ScriptDebugger::IsPaused() const {
        return std::any_of(mContexts.begin(), mContexts.end(), [](const ContextState& context) {
            return context.info.paused;
        });
    }

    bool ScriptDebugger::IsSelectedContextPaused() const {
        const auto* context = FindContext(mContext);
        return context != nullptr && context->info.paused;
    }

    bool ScriptDebugger::IsContextPaused(asIScriptContext* ctx) const {
        const auto* context = FindContext(ctx);
        return context != nullptr && context->info.paused;
    }

    std::vector<ScriptDebugger::ContextInfo> ScriptDebugger::GetContexts() const {
        std::vector<ContextInfo> contexts;
        contexts.reserve(mContexts.size());
        for (const auto& context : mContexts)
            contexts.push_back(context.info);
        return contexts;
    }

    void ScriptDebugger::SelectContext(size_t id) {
        for (auto& context : mContexts) {
            context.info.selected = context.info.id == id;
            if (context.info.selected) {
                mContext = context.context;
                mPaused = context.info.paused;
                mCurrentLocation = context.location;
            }
        }
    }

    void ScriptDebugger::SetContextPauseEnabled(size_t id, bool enabled) {
        for (auto& context : mContexts) {
            if (context.info.id == id) {
                context.info.pauseAtBreakpoints = enabled;
                return;
            }
        }
    }

    void ScriptDebugger::TakeCommands(asIScriptContext* ctx) {
        auto* state = FindContext(ctx);
        if (state == nullptr) {
            Attach(ctx);
            state = FindContext(ctx);
        }
        if (state == nullptr)
            return;

        for (auto& context : mContexts)
            context.info.selected = context.context == ctx;
        state->info.paused = true;
        state->location = mCurrentLocation;
        mContext = ctx;
        mPaused = true;
    }

    void ScriptDebugger::Output(const std::string& string) {
        mConsoleText += string + '\n';
    }

    void ScriptDebugger::LineCallback(asIScriptContext* ctx) {
        const auto* context = FindContext(ctx);
        if (context != nullptr && !context->info.pauseAtBreakpoints)
            return;

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

        mCurrentLocation.file = section;
        mCurrentLocation.line = line;
        mCurrentLocation.function = GetDisplayFunctionName(ctx->GetFunction());

        std::stringstream s;
        const char* file = 0;
        int lineNbr = ctx->GetLineNumber(0, 0, &file);
        s << (file ? file : "{unnamed}") << ":" << lineNbr << "; " << ctx->GetFunction()->GetDeclaration() << std::endl;
        Output(s.str());

        TakeCommands(ctx);
        ctx->Suspend();
    }

    void ScriptDebugger::AddFileBreakPoint(const std::string& file, int lineNbr) {
        if (file.empty() || lineNbr <= 0) {
            Output("A file breakpoint requires a file and a positive line number");
            return;
        }

        const auto existing = std::find_if(m_breakPoints.begin(), m_breakPoints.end(), [&file, lineNbr](const BreakPoint& bp) {
            return !bp.func && bp.name == file && bp.lineNbr == lineNbr;
        });
        if (existing != m_breakPoints.end())
            return;

        Output("Setting break point in file '" + file + "' at line " + std::to_string(lineNbr));
        m_breakPoints.emplace_back(file, lineNbr, false);
    }

    void ScriptDebugger::AddFuncBreakPoint(const std::string& func) {
        if (func.empty())
            return;
        const auto existing = std::find_if(m_breakPoints.begin(), m_breakPoints.end(), [&func](const BreakPoint& bp) {
            return bp.func && bp.name == func;
        });
        if (existing != m_breakPoints.end())
            return;

        Output("Adding deferred break point for function '" + func + "'");
        m_breakPoints.emplace_back(func, 0, true);
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

        bool enteredFunctionWithBreakpoint = false;
        if (m_lastFunction != func) {
            for (size_t i = 0; i < m_breakPoints.size(); ++i) {
                auto& bp = m_breakPoints[i];

                if (bp.func) {
                    if (bp.name == func->GetName() || bp.name == GetDisplayFunctionName(func) ||
                        bp.name == func->GetDeclaration()) {
                        std::stringstream s;
                        s << "Entering function breakpoint '" << bp.name << "'\n";
                        Output(s.str());
                        enteredFunctionWithBreakpoint = true;
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
        
        return enteredFunctionWithBreakpoint;
    }

    void ScriptDebugger::Continue() {
        auto* context = FindContext(mContext);
        if (context == nullptr || !context->info.paused)
            return;
        m_action = CONTINUE;
        context->info.paused = false;
        mPaused = false;
    }

    void ScriptDebugger::StepInto() {
        auto* context = FindContext(mContext);
        if (context == nullptr || !context->info.paused || !mContext)
            return;
        m_action = STEP_INTO;
        m_lastCommandAtStackLevel = mContext->GetCallstackSize();
        context->info.paused = false;
        mPaused = false;
    }

    void ScriptDebugger::StepOver() {
        auto* context = FindContext(mContext);
        if (context == nullptr || !context->info.paused || !mContext)
            return;
        m_action = STEP_OVER;
        m_lastCommandAtStackLevel = mContext->GetCallstackSize();
        context->info.paused = false;
        mPaused = false;
    }

    void ScriptDebugger::StepOut() {
        auto* context = FindContext(mContext);
        if (context == nullptr || !context->info.paused || !mContext)
            return;
        m_action = STEP_OUT;
        m_lastCommandAtStackLevel = mContext->GetCallstackSize();
        context->info.paused = false;
        mPaused = false;
    }

    std::optional<ScriptDebugger::SourceLocation> ScriptDebugger::GetCurrentLocation() const {
        if (!IsSelectedContextPaused() || mCurrentLocation.file.empty() || mCurrentLocation.line <= 0)
            return std::nullopt;
        return mCurrentLocation;
    }

    void ScriptDebugger::Detach(asIScriptContext* ctx) {
        if (auto* context = FindContext(ctx))
            context->info.paused = false;

        if (mContext != ctx)
            return;

        mContext = nullptr;
        mPaused = false;
        mCurrentLocation = {};
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
                    m_breakPoints.erase(m_breakPoints.begin() + static_cast<std::ptrdiff_t>(i));
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
                m_breakPoints.erase(m_breakPoints.begin() + static_cast<std::ptrdiff_t>(i));
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

        std::string internal_name = func->GetName();
        std::string declaration = func->GetDeclaration();

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

    void ScriptDebugger::Attach(asIScriptContext* ctx, std::string name) {
        if (ctx == nullptr)
            return;

        ContextState state;
        state.info.id = mNextContextId++;
        state.info.name = std::move(name);
        state.info.selected = mContexts.empty();
        state.context = ctx;
        mContexts.push_back(std::move(state));

        using LineCallback = void (*)(asIScriptContext*, void*);

        LineCallback callback = [](asIScriptContext* ctx, void* param) {
            auto* debugger = static_cast<ScriptDebugger*>(param);
            debugger->LineCallback(ctx);
        };

        const int result = ctx->SetLineCallback(
            asFunctionPtr(callback),
            this,
            asCALL_CDECL
        );

        Output(std::format("Attached to script context {:p}, with return {}", static_cast<void*>(ctx), result));
    }

    std::vector<ScriptDebugger::CallstackEntry> ScriptDebugger::GetCallStack() {
        std::vector<CallstackEntry> result;

        if (!mContext || !IsSelectedContextPaused()) {
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
        if (mContext == nullptr || !IsSelectedContextPaused()) {
            Output("No script running");
            return {};
        }

        asIScriptEngine* engine = mContext->GetEngine();

        engine->GetGCStatistics(&s.CurrentSize, &s.TotalDestructions, &s.TotalDetected, &s.NewObjects, &s.TotalNewDestructions);
        return s;
    }

    ScriptDebugger::DebugValue ScriptDebugger::CEToString(
        void *value,
        asUINT typeId,
        int expandMembers,
        asIScriptEngine *engine
    ) {
        DebugValue result;

        if (value == nullptr) {
            result.value = "<null>";
            return result;
        }

        if (engine == nullptr) {
            engine = m_engine;
        }

        std::stringstream s;

        if (typeId == asTYPEID_VOID) {
            result.value = "<void>";
        } else if (typeId == asTYPEID_BOOL) {
            result.value = *(bool *)value ? "true" : "false";
        } else if (typeId == asTYPEID_INT8) {
            s << (int)*(signed char *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_INT16) {
            s << (int)*(signed short *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_INT32) {
            s << *(signed int *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_INT64) {
    #if defined(_MSC_VER) && _MSC_VER <= 1200
            result.value = "{...}";
    #else
            s << *(asINT64 *)value;
            result.value = s.str();
    #endif
        } else if (typeId == asTYPEID_UINT8) {
            s << (unsigned int)*(unsigned char *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_UINT16) {
            s << (unsigned int)*(unsigned short *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_UINT32) {
            s << *(unsigned int *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_UINT64) {
    #if defined(_MSC_VER) && _MSC_VER <= 1200
            result.value = "{...}";
    #else
            s << *(asQWORD *)value;
            result.value = s.str();
    #endif
        } else if (typeId == asTYPEID_FLOAT) {
            s << *(float *)value;
            result.value = s.str();
        } else if (typeId == asTYPEID_DOUBLE) {
            s << *(double *)value;
            result.value = s.str();
        } else if ((typeId & asTYPEID_MASK_OBJECT) == 0) {
            // The type is an enum.
            s << *(asUINT *)value;
            result.value = s.str();

            if (engine) {
                asITypeInfo *type = engine->GetTypeInfoById(static_cast<int>(typeId));

                if (type) {
                    for (int n = static_cast<int>(type->GetEnumValueCount()); n-- > 0;) {
                        int enumVal;
                        const char *enumName = type->GetEnumValueByIndex(n, &enumVal);

                        if (enumVal == *(int *)value) {
                            result.value += ", ";
                            result.value += enumName;
                            break;
                        }
                    }
                }
            }
        } else if (typeId & asTYPEID_SCRIPTOBJECT) {
            // Dereference handles so we can inspect the object.
            if (typeId & asTYPEID_OBJHANDLE) {
                value = *(void **)value;
            }

            asIScriptObject *obj = (asIScriptObject *)value;

            // Print the address of the object.
            s << "{" << obj << "}";
            result.value = s.str();

            if (obj && expandMembers > 0) {
                asITypeInfo *type = obj->GetObjectType();

                for (asUINT n = 0; n < obj->GetPropertyCount(); n++) {
                    DebugValue member = CEToString(
                        obj->GetAddressOfProperty(n),
                        obj->GetPropertyTypeId(n),
                        expandMembers - 1,
                        type->GetEngine());

                    member.name = type->GetPropertyDeclaration(n);
                    result.members.push_back(std::move(member));
                }
            }
        } else {
            // Dereference handles so we can inspect the object.
            if (typeId & asTYPEID_OBJHANDLE) {
                value = *(void **)value;
            }

            if (engine) {
                asITypeInfo *type = engine->GetTypeInfoById(static_cast<int>(typeId));

                if (type) {
                    // Print the address for reference types so it is possible
                    // to see when handles point to the same object.
                    if (type->GetFlags() & asOBJ_REF) {
                        s << "{" << value << "}";
                        result.value = s.str();
                    }

                    if (value) {
                        // Check if there is a registered to-string callback.
                        auto it = m_toStringCallbacks.find(type);

                        if (it == m_toStringCallbacks.end()) {
                            // If the type is a template instance, there might be
                            // a to-string callback for the generic template type.
                            if (type->GetFlags() & asOBJ_TEMPLATE) {
                                asITypeInfo *tmplType =
                                    engine->GetTypeInfoByName(type->GetName());

                                it = m_toStringCallbacks.find(tmplType);
                            }
                        }

                        if (it != m_toStringCallbacks.end()) {
                            if (type->GetFlags() & asOBJ_REF) {
                                result.value += ' ';
                            }

                            // Invoke the callback to get the string representation.
                            result.value += it->second(value, expandMembers, this);
                        }
                    }
                }
            } else {
                result.value = "{no engine}";
            }
        }

        return result;
    }

    ScriptDebugger::DebugValue ScriptDebugger::GetMemberProperties() {
        if (mContext == nullptr || !IsSelectedContextPaused()) {
            Output("No script running");
            return {};
        }

        void* ptr = mContext->GetThisPointer();

        if (!ptr) {
            return {};
        }

        return CEToString(ptr, mContext->GetThisTypeId(), 3, mContext->GetEngine());
    }

    std::vector<ScriptDebugger::DebugValue> ScriptDebugger::GetLocalVariables() {
        std::vector<DebugValue> result;

        if (mContext == nullptr || !IsSelectedContextPaused()) {
            return result;
        }

        asIScriptFunction* func = mContext->GetFunction();
        if (!func) {
            return result;
        }

        for (asUINT n = 0; n < func->GetVarCount(); n++) {
            const char* name = nullptr;
            func->GetVar(n, &name);
            if (name == nullptr || strlen(name) == 0) {
                continue;
            }

            if (mContext->IsVarInScope(n)) {
                int typeId = 0;
                mContext->GetVar(n, 0, &name, &typeId);
                DebugValue var = CEToString(mContext->GetAddressOfVar(n), typeId, 3, mContext->GetEngine());
                var.name = GetFriendlySymbolName(name);
                result.push_back(std::move(var));
            }
        }

        return result;
    }

    std::vector<ScriptDebugger::DebugValue> ScriptDebugger::GetGlobalVariables() {
        std::vector<DebugValue> result;

        if (mContext == nullptr || !IsSelectedContextPaused()) {
            return result;
        }

        asIScriptFunction* func = mContext->GetFunction();
        if (!func) {
            return result;
        }

        asIScriptModule* mod = func->GetModule();
        if (!mod) {
            return result;
        }

        for (asUINT n = 0; n < mod->GetGlobalVarCount(); n++) {
            int typeId = 0;
            const char* varName = nullptr;
            mod->GetGlobalVar(n, &varName, nullptr, &typeId);

            DebugValue var = CEToString(mod->GetAddressOfGlobalVar(n), typeId, 3, mContext->GetEngine());
            const std::string internalName = varName ? varName : "";
            var.name = GetFriendlySymbolName(internalName);
            result.push_back(std::move(var));
        }

        return result;
    }

    std::string ScriptDebugger::GetFriendlySymbolName(const std::string& internalName) const {
        for (const auto& symbol : mSymbolmap) {
            if (symbol.InternalName == internalName) {
                return symbol.DisplayName.empty() ? internalName : symbol.DisplayName;
            }
        }

        return internalName;
    }
} // namespace CE::Scripting
