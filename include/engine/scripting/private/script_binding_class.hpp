#pragma once

#include <angelscript.h>

#include "engine/scripting/angelscript.hpp"

namespace CE::Scripting::Bindings {
    class IScriptBinding {
        public:
            IScriptBinding();
            virtual ~IScriptBinding() = default;
            
            virtual bool RegisterBindings(asIScriptEngine& script_engine, Runtime& runtime) = 0;
    };
}