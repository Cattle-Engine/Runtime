#pragma once

#include <string>
#include <string_view>

#include "engine/common/core/event_bus.hpp"

namespace CE::Core::GameState { 
    struct StateEnterEvent {
        std::string name;
    };

    struct StateExitEvent {
        std::string name;
    };

    struct StateChangedEvent {
        std::string from;
        std::string to;
    };

    class GameStateManager {
        public:
            explicit GameStateManager(EventBus& eventbus);

            void ChangeState(std::string state);
            void Emit(const std::string& eventName) const;
            bool IsState(std::string_view state) const;
            const std::string& GetState() const;
        private:
            std::string mCurrentState = "None";
            EventBus& mEventBus;
    };
}
