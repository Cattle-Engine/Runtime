#include <memory>
#include "engine/input/input_binder.hpp"
#include "engine/input/text.hpp"
#include "engine/instance.hpp"
#include "engine/common/tracelog.hpp"

namespace CE {
    int Instance::Bootstrap_InputManagers() {
        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating input managers", mInstanceID);
        
        mKeyboardManger = std::make_unique<CE::Input::Keyboard>(mInstanceWindowID);
        mMouseManger = std::make_unique<CE::Input::Mouse>(mInstanceWindowID);
        mTextInputManager = std::make_unique<CE::Input::TextInput>(mInstanceWindowID, *mWindow);
        mInputBindingManager = std::make_unique<CE::Input::Bindings::BindingManager>(*mVFS, *mKeyboardManger, *mMouseManger);
        return 0;
    }
}