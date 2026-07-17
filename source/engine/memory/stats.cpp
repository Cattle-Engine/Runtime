#include "engine/memory/stats.hpp"

namespace CE::Memory {
    Stats &GetStats() {
        static Stats stats;
        return stats;
    }
} // namespace CE::Memory