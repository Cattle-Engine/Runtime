#pragma once

#include "engine/scripting/angelscript.hpp"
#include <angelscript.h>

namespace CE::Scripting::Bindings {
    class IScriptBinding {
      public:
        IScriptBinding(Runtime& runtime, asIScriptEngine& script_engine) 
          : mRuntime(runtime), mScriptEngine(script_engine) {}
        virtual ~IScriptBinding() = default;
        virtual bool RegisterBindings() = 0;  
      protected:
          Runtime& mRuntime;
          asIScriptEngine& mScriptEngine;
    };
} // namespace CE::Scripting::Bindings