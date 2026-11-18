#include <memory>
#include "ce_runtime/input/input_binder.hpp"
#include "ce_runtime/input/text.hpp"
#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/common/tracelog.hpp"

namespace CE {
    int BaseInstance::Bootstrap_InputManagers() {
        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating input managers", mInstanceID);
        
        mKeyboardManger = std::make_unique<CE::Input::Keyboard>(mInstanceWindowID);
        mMouseManger = std::make_unique<CE::Input::Mouse>(mInstanceWindowID);
        mTextInputManager = std::make_unique<CE::Input::TextInput>(mInstanceWindowID, *mWindow);
        mInputBindingManager = std::make_unique<CE::Input::Bindings::BindingManager>(*mVFS, *mKeyboardManger, *mMouseManger);
        return 0;
    }
}