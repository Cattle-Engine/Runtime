#pragma once

#include "engine/scripting/bindings/script_binding_class.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/scripting/bindings/accs_native_types.hpp"

namespace CE::Scripting::Bindings {
    class ACCSBinding final : public IScriptBinding {
      public:
        using IScriptBinding::IScriptBinding;
        bool RegisterBindings() override;

        NativeTypeRegistry native_types;
    private:
        ACCS::ComponentID AddNativeComponent(
            const ACCS::ActorHandle& actor,
            const std::string& type
        );
    };
} // namespace CE::Scripting::Bindings
