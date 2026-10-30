#pragma once

#include "engine/scripting/bindings/script_binding_class.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/accs/accs.hpp"

namespace CE::Scripting::Bindings {
    class ACCSBinding final : public IScriptBinding {
      public:
        using IScriptBinding::IScriptBinding;
        bool RegisterBindings() override;
    };
} // namespace CE::Scripting::Bindings
