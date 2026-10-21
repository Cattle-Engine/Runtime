#pragma once

#include <cstdint>
#include <limits>

namespace CE::ACCS {
    constexpr uint32_t kInvalidComponentIndex = std::numeric_limits<uint32_t>::max();
    
    struct ComponentID {
        uint32_t index = kInvalidComponentIndex;
        uint32_t generation = 0;

        bool IsValid() const {
            return index != kInvalidComponentIndex;
        }

        bool operator==(const ComponentID& other) const {
            return index == other.index &&
                generation == other.generation;
        }

        bool operator!=(const ComponentID& other) const {
            return !(*this == other);
        }
    };

    class IComponent {
        public:
            virtual ~IComponent() = default;
    };
}