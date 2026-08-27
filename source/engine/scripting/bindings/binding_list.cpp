#include "engine/scripting/bindings/bindings_list.hpp"
#include "engine/scripting/bindings/stl_vector_binder.hpp"

namespace CE::Scripting::Bindings {
    bool ScriptBindings::RegisterAllBindings(asIScriptEngine& script_engine, Runtime& runtime) {
        script_engine.SetDefaultNamespace("CE::Containers");

        RegisterVector<asUINT>(&script_engine, "Uint32Vector", "uint32");

        script_engine.SetDefaultNamespace("");

        return true;
    }
} // namespace CE::Scripting::Bindings