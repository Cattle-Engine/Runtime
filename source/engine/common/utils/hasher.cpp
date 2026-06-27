#include <xxhash.h>

#include "engine/common/utils/hasher.hpp"

namespace CE::Utils {
    uint64_t Hash64(const void* data, std::size_t size) {
        return XXH3_64bits(data, size);
    }
 
    uint64_t Hash64(const std::string& string) {
        return XXH3_64bits(string.c_str(), string.size());
    }
}   