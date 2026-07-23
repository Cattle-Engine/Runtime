#include "engine/scripting/bindings/script_binding_class.hpp"

namespace CE::Scripting::Bindings {
    class TextureResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
    };
    
    class MaterialResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
    };
    
    class GPUMeshResourceBindings : public IScriptBinding {
        public:
            bool RegisterBindings() override;
    };
}