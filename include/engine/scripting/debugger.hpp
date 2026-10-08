#pragma once

#include <string>
#include <vector>
#include <angelscript.h>
#include <debugger/debugger.h>
#include <unordered_map>
#include <memory>
#include <optional>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting {
    class ScriptDebugger : public CDebugger {
    public:
        explicit ScriptDebugger(std::vector<Impl::GeneratedSymbolInfo> symbol_map = {}) {
            Output("Starting debugger");
            SetGeneratedSymbols(std::move(symbol_map));
        }

        void SetGeneratedSymbols(std::vector<Impl::GeneratedSymbolInfo> symbol_map) {
            mSymbolmap = std::move(symbol_map);
            mGeneratedSymbolLookup.clear();
            mGeneratedSymbolQualifiedLookup.clear();

            for (const auto& symbol : mSymbolmap) {
                if (!symbol.InternalName.empty()) {
                    mGeneratedSymbolLookup[symbol.InternalName] = symbol.DisplayName;
                }

                if (!symbol.QualifiedName.empty()) {
                    mGeneratedSymbolLookup[symbol.QualifiedName] = symbol.DisplayName;
                    mGeneratedSymbolQualifiedLookup[symbol.QualifiedName] = symbol.DisplayName;
                }

                if (!symbol.DisplayName.empty()) {
                    mGeneratedSymbolLookup[symbol.DisplayName] = symbol.DisplayName;
                }
            }
        }

        struct BreakPointInfo {
            std::string name;
            int line;
            bool function;
        };

        struct CallstackEntry {
            std::string file;
            int line;
            std::string function;
        };

        struct SourceLocation {
            std::string file;
            int line = 0;
            std::string function;
        };

        struct ContextInfo {
            size_t id = 0;
            std::string name;
            bool pauseAtBreakpoints = true;
            bool paused = false;
            bool selected = false;
        };

        struct GCStatistics {
            asUINT CurrentSize;
            asUINT TotalDestructions;
            asUINT TotalDetected;
            asUINT NewObjects;
            asUINT TotalNewDestructions;
        };

        struct DebugValue {
            std::string name;
            std::string value;
            std::vector<DebugValue> members;
        };

        std::vector<CallstackEntry> GetCallStack();
        GCStatistics GetGCStats();
        DebugValue GetMemberProperties();
        std::vector<DebugValue> GetLocalVariables();
        std::vector<DebugValue> GetGlobalVariables();
        void TakeCommands(asIScriptContext* ctx) override;
        void Output(const std::string& string) override;
        // based off of the CDebugger base just modified to fit CE better
        void LineCallback(asIScriptContext* ctx) override;

        bool IsPaused() const;

        bool IsSelectedContextPaused() const;
        bool IsContextPaused(asIScriptContext* ctx) const;
        std::vector<ContextInfo> GetContexts() const;
        void SelectContext(size_t id);
        void SetContextPauseEnabled(size_t id, bool enabled);

        std::optional<SourceLocation> GetCurrentLocation() const;
        // Must be called by the runtime once a suspended context has either
        // finished or is about to be released.
        void Detach(asIScriptContext* ctx);

        const std::string& GetOutputLog() {
            return mConsoleText;
        }

        asIScriptContext* GetContext() const {
            return mContext;
        }

        void Continue();
        void StepInto();
        void StepOver();
        void StepOut();

        // overridden so we can store the full file path in BreakPoint.name
        // also based off of the CDebugger base just modified to fit CE better
        void AddFileBreakPoint(const std::string &file, int lineNbr) override;
        void AddFuncBreakPoint(const std::string &func) override;
        bool CheckBreakPoint(asIScriptContext *ctx) override;

        std::vector<BreakPointInfo> GetBreakPoints() const;
        bool RemoveFileBreakPoint(const std::string& file, int line);
        bool RemoveFuncBreakPoint(const std::string& func);

        std::string GetDisplayFunctionName(asIScriptFunction* func) const;

        void Attach(asIScriptContext* ctx, std::string name = "Script");
    private:
        struct ContextState {
            ContextInfo info;
            asIScriptContext* context = nullptr;
            SourceLocation location;
        };

        struct Location {
            std::string name;
            int line = 0;
            bool function = false;
        };

        DebugValue CEToString(void *value, asUINT typeId, int expandMembers, asIScriptEngine *engine);
        ContextState* FindContext(asIScriptContext* ctx);
        const ContextState* FindContext(asIScriptContext* ctx) const;

        std::unordered_map<std::string, std::string> mGeneratedSymbolLookup;
        std::unordered_map<std::string, std::string> mGeneratedSymbolQualifiedLookup;

        std::string mCurrentFile;
        int mCurrentLine = 0;
        asIScriptContext* mContext = nullptr;
        bool mPaused = false;
        SourceLocation mCurrentLocation;
        std::vector<ContextState> mContexts;
        size_t mNextContextId = 1;
        int mStepStackDepth = 0;
        std::string mConsoleText;

        std::vector<Impl::GeneratedSymbolInfo> mSymbolmap;
    };

    using SharedScriptDebuger = std::shared_ptr<ScriptDebugger>;
}
