#pragma once

#include <angelscript.h>

namespace CE::Scripting::Bindings {
    class IScriptBinding {
        public:
            IScriptBinding();
            virtual ~IScriptBinding() = default;
            
            virtual bool RegisterBindings(asIScriptEngine& script_engine) = 0;
    };
}