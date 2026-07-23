#pragma once

#include <memory>

#include "engine/scripting/angelscript.hpp"
#include <angelscript.h>

namespace CE::Scripting::Bindings {
    // This class and the PImpl is to try and hide bindings from the Runtime class
    class ScriptBindings {
        public:
            bool RegisterAllBindings(asIScriptEngine& script_engine, Runtime& runtime);
        private:
            class Impl;
            std::unique_ptr<Impl> mImpl;
    };
}