#include "engine/scripting/angelscript.hpp"

namespace CE::Scripting {
    bool Runtime::RegisterAssetsBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        if (!RegisterAssetCoreBindings()) {
            return false;
        }

        if (!RegisterAssetMeshBindings()) {
            return false;
        }

        if (!RegisterAssetTextureBindings()) {
            return false;
        }

        if (!RegisterAssetShaderBindings()) {
            return false;
        }

        if (!RegisterAssetPrimitiveBindings()) {
            return false;
        }

        if (!RegisterAssetAnimationBindings()) {
            return false;
        }

        return true;
    }
}
