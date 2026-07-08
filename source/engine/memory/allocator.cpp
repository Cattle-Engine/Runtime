#include <engine/memory/allocator.hpp>
#include <engine/memory/stats.hpp>
#include <engine/platforms.hpp>

#include <atomic>
#include <cstdlib>
#include <new>

namespace {
    std::atomic<bool> trackingEnabled{false};

    void TrackAllocation(std::size_t size) {
        if (!CE::Memory::IsTrackingEnabled())
            return;

        auto& stats = CE::Memory::GetStats();

        stats.allocations.fetch_add(1, std::memory_order_relaxed);
        stats.bytesAllocated.fetch_add(size, std::memory_order_relaxed);

        auto current = stats.currentBytes.fetch_add(
            size,
            std::memory_order_relaxed) + size;

        auto peak = stats.peakBytes.load(std::memory_order_relaxed);

        while (current > peak &&
            !stats.peakBytes.compare_exchange_weak(
                peak,
                current,
                std::memory_order_relaxed)) {
        }
    }

    void TrackDeallocation(std::size_t size = 0) {
        if (!CE::Memory::IsTrackingEnabled())
            return;

        auto& stats = CE::Memory::GetStats();

        stats.deallocations.fetch_add(1, std::memory_order_relaxed);

        if (size)
            stats.bytesFreed.fetch_add(size, std::memory_order_relaxed);
    }
}

namespace CE::Memory {
    void EnableTracking(bool enabled) {
        trackingEnabled.store(enabled, std::memory_order_relaxed);
    }

    bool IsTrackingEnabled() {
        return trackingEnabled.load(std::memory_order_relaxed);
    }
}

void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);

    if (!ptr)
        throw std::bad_alloc();

    TrackAllocation(size);

    return ptr;
}

void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);

    if (!ptr)
        throw std::bad_alloc();

    TrackAllocation(size);

    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (!ptr)
        return;

    TrackDeallocation();

    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    if (!ptr)
        return;

    TrackDeallocation();

    std::free(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    if (!ptr)
        return;

    TrackDeallocation(size);

    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    if (!ptr)
        return;

    TrackDeallocation(size);

    std::free(ptr);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return ::operator new(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return ::operator new[](size);
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    ::operator delete[](ptr);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    void* ptr = CE::Platforms::AlignedAllocate(
        size,
        static_cast<std::size_t>(alignment));

    if (!ptr)
        throw std::bad_alloc();

    TrackAllocation(size);

    return ptr;
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    void* ptr = CE::Platforms::AlignedAllocate(
        size,
        static_cast<std::size_t>(alignment));

    if (!ptr)
        throw std::bad_alloc();

    TrackAllocation(size);

    return ptr;
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    if (!ptr)
        return;

    TrackDeallocation();

    CE::Platforms::AlignedFree(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    if (!ptr)
        return;

    TrackDeallocation();

    CE::Platforms::AlignedFree(ptr);
}

void operator delete(void* ptr, std::size_t size, std::align_val_t) noexcept {
    if (!ptr)
        return;

    TrackDeallocation(size);

    CE::Platforms::AlignedFree(ptr);
}

void operator delete[](void* ptr, std::size_t size, std::align_val_t) noexcept {
    if (!ptr)
        return;

    TrackDeallocation(size);

    CE::Platforms::AlignedFree(ptr);
}