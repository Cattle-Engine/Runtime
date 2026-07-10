#include <xxhash.h>
#include <sstream>
#include <iomanip>

#include "engine/common/utils/hasher.hpp"

namespace CE::Utils {
    uint64_t Hash64(const void* data, std::size_t size) {
        return XXH3_64bits(data, size);
    }
 
    uint64_t Hash64(const std::string& string) {
        return XXH3_64bits(string.c_str(), string.size());
    }

     std::string Hash2String(uint64_t hash) {
        std::stringstream ss;

        ss << std::hex << std::setfill('0') << std::setw(0) << hash;

        return ss.str();
    }
}   
