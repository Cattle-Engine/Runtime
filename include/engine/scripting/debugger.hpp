#pragma once

#include <string>
#include <vector>
#include <angelscript.h>
#include <debugger/debugger.h>
#include <unordered_map>
#include <memory>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting {
    class ScriptDebugger : public CDebugger {
    public:
        explicit ScriptDebugger(std::vector<Impl::GeneratedSymbolInfo> symbol_map = {}) {
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

        void TakeCommands(asIScriptContext* ctx) override;
        void Output(const std::string& string) override;
        // based off of the CDebugger base just modified to fit CE better
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

        // overridden so we can store the full file path in BreakPoint.name
        // also based off of the CDebugger base just modified to fit CE better
        void AddFileBreakPoint(const std::string &file, int lineNbr) override;
        bool CheckBreakPoint(asIScriptContext *ctx) override;

        std::vector<BreakPointInfo> GetBreakPoints() const;
        bool RemoveFileBreakPoint(const std::string& file, int line);
        bool RemoveFuncBreakPoint(const std::string& func);

        std::string GetDisplayFunctionName(asIScriptFunction* func) const;

        void Attach(asIScriptContext* ctx);
    private:
        struct Location {
            std::string name;
            int line = 0;
            bool function = false;
        };

        std::unordered_map<std::string, std::string> mGeneratedSymbolLookup;
        std::unordered_map<std::string, std::string> mGeneratedSymbolQualifiedLookup;

        std::string mCurrentFile;
        int mCurrentLine = 0;
        asIScriptContext* mContext = nullptr;
        bool mPaused = false;
        int mStepStackDepth = 0;
        std::string mConsoleText;

        std::vector<Impl::GeneratedSymbolInfo> mSymbolmap;
    };

    using SharedScriptDebuger = std::shared_ptr<ScriptDebugger>;
}