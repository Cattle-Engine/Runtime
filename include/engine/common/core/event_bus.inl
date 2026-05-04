#pragma once

#include "engine/common/core/event_bus.hpp"

namespace CE::Core {

    template<typename Event>
    int EventBus::Subscribe(Handler<Event> handler) {
        auto& vec = handlers[typeid(Event)];

        vec.emplace_back(
            [handler](const void* e) {
                handler(*static_cast<const Event*>(e));
            }
        );

        return static_cast<int>(vec.size() - 1);
    }

    template<typename Event>
    void EventBus::Emit(const Event& event) {
        auto it = handlers.find(typeid(Event));
        if (it == handlers.end()) return;

        for (auto& fn : it->second) {
            if (fn) fn(&event);
        }
    }

    template<typename Event>
    void EventBus::Unsubscribe(int id) {
        auto it = handlers.find(typeid(Event));
        if (it == handlers.end()) return;

        auto& vec = it->second;

        if (id >= 0 && id < static_cast<int>(vec.size())) {
            vec[id] = nullptr;
        }
    }

}