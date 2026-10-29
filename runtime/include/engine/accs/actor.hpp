#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

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
        // Metadata (persisted by WorldSerializer)
        std::string name;
        bool active = true;
        std::vector<std::string> tags; // unique, insertion order

        // Runtime structure (rebuilt on load)
        std::vector<ComponentID> components;
        std::unordered_map<ComponentTypeKey, ComponentID> component_lookup;
    };
}