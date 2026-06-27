#pragma once

#include <cstdint>
#include <string>

namespace CE::Utils {
    uint64_t Hash64(const void* data, std::size_t size);
    uint64_t Hash64(const std::string& string);
}