#pragma once

#include <memory>

#include "ce_runtime/scripting/angelscript.hpp"

#include <angelscript.h>

namespace CE::Scripting::Bindings {
    // This class and the PImpl is to try and hide bindings from the Runtime class
    class ScriptBindings {
      public:
        bool RegisterAllBindings(asIScriptEngine& script_engine, Runtime& runtime);
        ~ScriptBindings();
      private:
        class Impl;
        Impl* mImpl = nullptr;
    };
} // namespace CE::Scripting::Bindings