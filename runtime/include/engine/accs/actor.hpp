#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <typeindex>
#include <unordered_map>

#include "engine/accs/component.hpp"

namespace CE::ACCS {
    constexpr uint32_t kInvalidActorHandleIndex = std::numeric_limits<uint32_t>::max();

    struct ActorHandle {
        uint32_t index = kInvalidActorHandleIndex;
        uint32_t generation = 0;

        bool IsValid() const {
            return index != kInvalidActorHandleIndex;
        }

        bool operator==(const ActorHandle&) const = default;
    };

    struct Actor {
        std::vector<ComponentID> components;
        std::unordered_map<std::type_index, ComponentID> component_lookup;
    };
}