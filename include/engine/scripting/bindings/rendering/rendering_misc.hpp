#pragma once

#include "engine/scripting/bindings/script_binding_class.hpp"

namespace CE::Scripting::Bindings {
    // Stuff that I don't know where to give a home
    class RenderingMisc : public IScriptBinding {
        public:
            bool RegisterBindings() override;
        private:
            // Registers CE::Renderer::Colour
            bool RegisterColourBinding();
    };
}