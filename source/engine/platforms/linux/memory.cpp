#include <cstdlib>

#include "engine/platforms/linux.hpp"

namespace CE::Platforms::Linux {
    void *AlignedAllocate(std::size_t size, std::size_t alignment) {
        void *ptr = nullptr;

        if (posix_memalign(&ptr, alignment, size) != 0)
            return nullptr;

        return ptr;
    }

    void AlignedFree(void *ptr) noexcept {
        std::free(ptr);
    }
} // namespace CE::Platforms::Linux