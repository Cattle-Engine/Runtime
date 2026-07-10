#pragma once

#include <atomic>
#include <cstddef>

namespace CE::Memory {
    struct Stats {
        std::atomic<std::size_t> allocations{0};
        std::atomic<std::size_t> deallocations{0};
        std::atomic<std::size_t> bytesAllocated{0};
        std::atomic<std::size_t> bytesFreed{0};
        std::atomic<std::size_t> currentBytes{0};
        std::atomic<std::size_t> peakBytes{0};
        std::atomic<std::size_t> aliveAllocations{0};
        std::atomic<std::size_t> peakAliveAllocations{0};
    };

    Stats& GetStats();
}