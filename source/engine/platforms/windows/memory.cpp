#include <malloc.h>
#include <cstdlib>

#include "engine/platforms/windows.hpp"

namespace CE::Platforms::Windows {
    void* AlignedAllocate(std::size_t size, std::size_t alignment) {
        return _aligned_malloc(size, alignment);
    }

    void AlignedFree(void* ptr) noexcept {
        _aligned_free(ptr);
    }
}