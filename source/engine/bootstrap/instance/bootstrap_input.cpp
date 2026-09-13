#include <memory>
#include "engine/input/text.hpp"
#include "engine/instance.hpp"
#include "engine/common/tracelog.hpp"

namespace CE {
    int Instance::Bootstrap_InputManagers() {
        CE_LOG(CE::LogLevel::Info, "[Instance {}] Creating input managers", gInstanceID);
        
        mKeyboardManger = std::make_unique<CE::Input::Keyboard>(gInstanceWindowID);
        mMouseManger = std::make_unique<CE::Input::Mouse>(gInstanceWindowID);
        mTextInputManager = std::make_unique<CE::Input::TextInput>(gInstanceWindowID, *mWindow);
        return 0;
    }
}