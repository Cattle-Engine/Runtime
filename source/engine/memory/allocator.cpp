#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <new>
#include <unordered_map>

#include <engine/memory/allocator.hpp>
#include <engine/memory/stats.hpp>
#include <engine/platforms.hpp>

namespace {

    constexpr std::uint64_t AllocationMagic = 0xCECAFEBABE123456ULL;

    struct AllocationHeader {
        std::uint64_t magic;
        std::size_t size;
        void* original;
    };

    std::atomic<bool> trackingEnabled{false};

    template <typename T> struct MallocAllocator {
        using value_type = T;

        MallocAllocator() = default;
        template <typename U> constexpr MallocAllocator(const MallocAllocator<U>&) noexcept {}

        T* allocate(std::size_t n) {
            if (n > std::size_t(-1) / sizeof(T))
                throw std::bad_alloc();
            if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
                return p;
            throw std::bad_alloc();
        }

        void deallocate(T* p, std::size_t) noexcept {
            std::free(p);
        }

        template <typename U> bool operator==(const MallocAllocator<U>&) const {
            return true;
        }
        template <typename U> bool operator!=(const MallocAllocator<U>&) const {
            return false;
        }
    };

    using AllocationMap = std::unordered_map<void*, AllocationHeader*, std::hash<void*>, std::equal_to<void*>,
                                             MallocAllocator<std::pair<void* const, AllocationHeader*>>>;

    AllocationMap& GetAllocationMap() {
        static AllocationMap map;
        return map;
    }

    std::mutex& GetAllocationMutex() {
        static std::mutex mtx;
        return mtx;
    }

    void TrackAllocation(std::size_t size) {
        if (!CE::Memory::IsTrackingEnabled())
            return;

        auto& stats = CE::Memory::GetStats();

        stats.allocations.fetch_add(1, std::memory_order_relaxed);
        stats.bytesAllocated.fetch_add(size, std::memory_order_relaxed);

        auto alive = stats.aliveAllocations.fetch_add(1, std::memory_order_relaxed) + 1;

        auto peakAlive = stats.peakAliveAllocations.load(std::memory_order_relaxed);

        while (alive > peakAlive &&
               !stats.peakAliveAllocations.compare_exchange_weak(peakAlive, alive, std::memory_order_relaxed)) {
        }

        auto current = stats.currentBytes.fetch_add(size, std::memory_order_relaxed) + size;

        auto peak = stats.peakBytes.load(std::memory_order_relaxed);

        while (current > peak && !stats.peakBytes.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
        }
    }

    void TrackDeallocation(std::size_t size) {
        if (!CE::Memory::IsTrackingEnabled())
            return;

        auto& stats = CE::Memory::GetStats();

        stats.deallocations.fetch_add(1, std::memory_order_relaxed);
        stats.bytesFreed.fetch_add(size, std::memory_order_relaxed);
        stats.currentBytes.fetch_sub(size, std::memory_order_relaxed);
        stats.aliveAllocations.fetch_sub(1, std::memory_order_relaxed);
    }

    void RegisterAllocation(void* ptr, AllocationHeader* header) {
        std::scoped_lock lock(GetAllocationMutex());
        GetAllocationMap().emplace(ptr, header);
    }

    AllocationHeader* RemoveAllocation(void* ptr) {
        std::scoped_lock lock(GetAllocationMutex());

        auto& map = GetAllocationMap();
        auto it = map.find(ptr);

        if (it == map.end())
            return nullptr;

        auto* header = it->second;
        map.erase(it);
        return header;
    }

    void* AllocateAligned(std::size_t size, std::size_t alignment) {
        const auto total = size + alignment + sizeof(AllocationHeader);

        void* raw = CE::Platforms::AlignedAllocate(total, alignment);

        if (!raw)
            throw std::bad_alloc();

        auto address = reinterpret_cast<std::uintptr_t>(raw);

        auto aligned = (address + sizeof(AllocationHeader) + alignment - 1) & ~(alignment - 1);

        auto* header = reinterpret_cast<AllocationHeader*>(aligned - sizeof(AllocationHeader));

        header->magic = AllocationMagic;
        header->size = size;
        header->original = raw;

        void* result = reinterpret_cast<void*>(aligned);

        RegisterAllocation(result, header);

        TrackAllocation(size);

        return result;
    }

} // namespace

namespace CE::Memory {
    void EnableTracking(bool enabled) {
        trackingEnabled.store(enabled, std::memory_order_relaxed);
    }

    bool IsTrackingEnabled() {
        return trackingEnabled.load(std::memory_order_relaxed);
    }

} // namespace CE::Memory

void* operator new(std::size_t size) {
    const auto total = sizeof(AllocationHeader) + size;

    void* raw = std::malloc(total);

    if (!raw)
        throw std::bad_alloc();

    auto* header = reinterpret_cast<AllocationHeader*>(raw);

    header->magic = AllocationMagic;
    header->size = size;
    header->original = raw;

    void* ptr = reinterpret_cast<void*>(header + 1);

    RegisterAllocation(ptr, header);

    TrackAllocation(size);

    return ptr;
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
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

void* operator new(std::size_t size, std::align_val_t alignment) {
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try {
        return ::operator new(size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try {
        return ::operator new[](size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void* ptr) noexcept {
    if (!ptr)
        return;

    auto* header = RemoveAllocation(ptr);

    if (!header) {
        std::free(ptr);
        return;
    }

    TrackDeallocation(header->size);

    std::free(header);
}

void operator delete[](void* ptr) noexcept {
    ::operator delete(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    ::operator delete[](ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    if (!ptr)
        return;

    auto* header = RemoveAllocation(ptr);

    if (!header)
        return;

    TrackDeallocation(header->size);

    CE::Platforms::AlignedFree(header->original);
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

void operator delete(void* ptr, std::size_t, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t alignment) noexcept {
    operator delete[](ptr, alignment);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    ::operator delete[](ptr);
}

void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    operator delete(ptr, alignment);
}

void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    operator delete[](ptr, alignment);
}