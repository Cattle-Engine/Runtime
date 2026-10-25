#pragma once

#include "engine/scripting/bindings/script_binding_class.hpp"

namespace CE::Scripting::Bindings {
    class ACCSBinding final : public IScriptBinding {
      public:
        using IScriptBinding::IScriptBinding;
        bool RegisterBindings() override;
    };
} // namespace CE::Scripting::Bindings
