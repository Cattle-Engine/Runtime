#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <typeindex>
#include <vector>

namespace CE::Core {
    class EventBus {
        public:
            template<typename Event>
            using Handler = std::function<void(const Event&)>;
            using StateHandler = std::function<void(std::string_view, std::string_view)>;

            template<typename Event>
            int Subscribe(Handler<Event> handler);

            template<typename Event>
            void Emit(const Event& event);

            template<typename Event>
            void Unsubscribe(int id);

            int Subscribe(const std::string& state, const std::string& eventName, StateHandler handler);
            void Emit(std::string_view state, std::string_view eventName);
            void Unsubscribe(const std::string& state, const std::string& eventName, int id);

    private:
        using ErasedHandler = std::function<void(const void*)>;
        struct StateKey {
            std::string state;
            std::string eventName;

            bool operator==(const StateKey& other) const;
        };

        struct StateKeyHash {
            std::size_t operator()(const StateKey& key) const;
        };

        void EmitBucket(std::string_view state, std::string_view eventName, std::string_view emittedState, std::string_view emittedEventName);

        std::unordered_map<std::type_index, std::vector<ErasedHandler>> handlers;
        std::unordered_map<StateKey, std::vector<StateHandler>, StateKeyHash> stateHandlers;
    };

}

#include "engine/common/core/event_bus.inl"
