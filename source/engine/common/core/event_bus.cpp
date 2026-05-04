#include "engine/common/core/event_bus.hpp"

namespace CE::Core {
    bool EventBus::StateKey::operator==(const StateKey& other) const {
        return state == other.state && eventName == other.eventName;
    }

    std::size_t EventBus::StateKeyHash::operator()(const StateKey& key) const {
        const std::size_t stateHash = std::hash<std::string>{}(key.state);
        const std::size_t eventHash = std::hash<std::string>{}(key.eventName);
        return stateHash ^ (eventHash << 1);
    }

    int EventBus::Subscribe(const std::string& state, const std::string& eventName, StateHandler handler) {
        auto& vec = stateHandlers[StateKey{state, eventName}];
        vec.emplace_back(std::move(handler));
        return static_cast<int>(vec.size() - 1);
    }

    void EventBus::Emit(std::string_view state, std::string_view eventName) {
        EmitBucket(state, eventName, state, eventName);
        EmitBucket("*", eventName, state, eventName);
        EmitBucket(state, "*", state, eventName);
        EmitBucket("*", "*", state, eventName);
    }

    void EventBus::Unsubscribe(const std::string& state, const std::string& eventName, int id) {
        auto it = stateHandlers.find(StateKey{state, eventName});
        if (it == stateHandlers.end()) {
            return;
        }

        auto& vec = it->second;
        if (id >= 0 && id < static_cast<int>(vec.size())) {
            vec[id] = nullptr;
        }
    }

    void EventBus::EmitBucket(
        std::string_view state,
        std::string_view eventName,
        std::string_view emittedState,
        std::string_view emittedEventName
    ) {
        auto it = stateHandlers.find(StateKey{std::string(state), std::string(eventName)});
        if (it == stateHandlers.end()) {
            return;
        }

        for (auto& fn : it->second) {
            if (fn) {
                fn(emittedState, emittedEventName);
            }
        }
    }
}
