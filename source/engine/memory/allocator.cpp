#include <engine/memory/allocator.hpp>
#include <engine/memory/stats.hpp>
#include <engine/platforms.hpp>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>

struct AllocationHeader {
    std::size_t size;
};

namespace {
    std::atomic<bool> trackingEnabled{false};

    void TrackAllocation(std::size_t size) {
        if (!CE::Memory::IsTrackingEnabled()) return;

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

    void TrackDeallocation(std::size_t size) {
        if (!CE::Memory::IsTrackingEnabled())
            return;

        auto& stats = CE::Memory::GetStats();

        stats.deallocations.fetch_add(1, std::memory_order_relaxed);
        stats.bytesFreed.fetch_add(size, std::memory_order_relaxed);
        stats.currentBytes.fetch_sub(size, std::memory_order_relaxed);
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
    const auto total_size = sizeof(AllocationHeader) + size;

    void* raw = std::malloc(total_size);

    if (!raw) throw std::bad_alloc();

    auto* header = static_cast<AllocationHeader*>(raw);
    header->size = size;

    TrackAllocation(size);

    return header + 1;
}

void* operator new[](std::size_t size) {
    const auto total_size = sizeof(AllocationHeader) + size;

    void* raw = std::malloc(total_size);

    if (!raw)
        throw std::bad_alloc();

    auto* header = static_cast<AllocationHeader*>(raw);
    header->size = size;

    TrackAllocation(size);

    return header + 1;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;

    auto* header = static_cast<AllocationHeader*>(ptr) - 1;

    TrackDeallocation(header->size);

    std::free(header);
}

void operator delete[](void* ptr) noexcept {
    if (!ptr)
        return;

    auto* header = static_cast<AllocationHeader*>(ptr) - 1;

    TrackDeallocation(header->size);

    std::free(header);
}

void operator delete(void* ptr, std::size_t) noexcept {
    if (!ptr)
        return;

    auto* header = static_cast<AllocationHeader*>(ptr) - 1;

    TrackDeallocation(header->size);

    std::free(header);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    if (!ptr)
        return;

    auto* header = static_cast<AllocationHeader*>(ptr) - 1;

    TrackDeallocation(header->size);

    std::free(header);
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
    const auto align = static_cast<std::size_t>(alignment);
    const auto header_size = sizeof(AllocationHeader);
    const auto ptr_size = sizeof(void*);
    const auto total_size = size + align + header_size + ptr_size;

    void* raw = CE::Platforms::AlignedAllocate(total_size, align);

    if (!raw)
        throw std::bad_alloc();

    auto raw_addr = reinterpret_cast<std::uintptr_t>(raw);
    auto aligned_user_addr = (raw_addr + ptr_size + header_size + align - 1) & ~(align - 1);

    auto* header = reinterpret_cast<AllocationHeader*>(aligned_user_addr - header_size);
    header->size = size;

    void** original_ptr = reinterpret_cast<void**>(aligned_user_addr - header_size - ptr_size);
    *original_ptr = raw;

    TrackAllocation(size);

    return reinterpret_cast<void*>(aligned_user_addr);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    const auto align = static_cast<std::size_t>(alignment);
    const auto header_size = sizeof(AllocationHeader);
    const auto ptr_size = sizeof(void*);
    const auto total_size = size + align + header_size + ptr_size;

    void* raw = CE::Platforms::AlignedAllocate(total_size, align);

    if (!raw)
        throw std::bad_alloc();

    auto raw_addr = reinterpret_cast<std::uintptr_t>(raw);
    auto aligned_user_addr = (raw_addr + ptr_size + header_size + align - 1) & ~(align - 1);

    auto* header = reinterpret_cast<AllocationHeader*>(aligned_user_addr - header_size);
    header->size = size;

    void** original_ptr = reinterpret_cast<void**>(aligned_user_addr - header_size - ptr_size);
    *original_ptr = raw;

    TrackAllocation(size);

    return reinterpret_cast<void*>(aligned_user_addr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    if (!ptr)
        return;

    auto* header = reinterpret_cast<AllocationHeader*>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader));
    auto* original_ptr = reinterpret_cast<void**>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader) - sizeof(void*));

    TrackDeallocation(header->size);

    CE::Platforms::AlignedFree(*original_ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    if (!ptr)
        return;

    auto* header = reinterpret_cast<AllocationHeader*>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader));
    auto* original_ptr = reinterpret_cast<void**>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader) - sizeof(void*));

    TrackDeallocation(header->size);

    CE::Platforms::AlignedFree(*original_ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    if (!ptr)
        return;

    auto* header = reinterpret_cast<AllocationHeader*>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader));
    auto* original_ptr = reinterpret_cast<void**>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader) - sizeof(void*));

    TrackDeallocation(header->size);

    CE::Platforms::AlignedFree(*original_ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    if (!ptr)
        return;

    auto* header = reinterpret_cast<AllocationHeader*>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader));
    auto* original_ptr = reinterpret_cast<void**>(
        reinterpret_cast<char*>(ptr) - sizeof(AllocationHeader) - sizeof(void*));

    TrackDeallocation(header->size);

    CE::Platforms::AlignedFree(*original_ptr);
}