#pragma once

#include "engine/scripting/angelscript.hpp"

#include <angelscript.h>

namespace CE::Scripting::Bindings {
    class IScriptBinding {
      public:
        IScriptBinding();
        virtual ~IScriptBinding() = default;

        virtual bool RegisterBindings(asIScriptEngine& script_engine, Runtime& runtime) = 0;
    };
} // namespace CE::Scripting::Bindings