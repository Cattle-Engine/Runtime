#include "engine/scripting/bindings/game_state.hpp"

namespace CE::Scripting::Bindings {
    int RegisterSubscribeEventFuncDef(asIScriptEngine& script_engine) {
        script_engine.SetDefaultNamespace("CE::GameState");
        return script_engine.RegisterFuncdef("void StateEventCallback(const string& in, const string& in)");
    }
}