#pragma once

#include "ce_runtime/scripting/bindings/script_binding_class.hpp"
#include "ce_runtime/common/tracelog.hpp"
#include "ce_runtime/accs/accs.hpp"

namespace CE::Scripting::Bindings {
    class ACCSBinding final : public IScriptBinding {
      public:
        using IScriptBinding::IScriptBinding;
        bool RegisterBindings() override;
    };
} // namespace CE::Scripting::Bindings
