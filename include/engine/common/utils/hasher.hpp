#pragma once

#include <cstdint>
#include <string>

namespace CE::Utils {
    class StreamingHasher {
        public:
            StreamingHasher();
            ~StreamingHasher();
            
            void AddData(const void* data, std::size_t size);
            void AddString(const std::string& string);
            void AddValue(auto&& value);
            uint64_t Finalize();
        private:
            class Impl;
            Impl* mImpl;
    };
    
    uint64_t Hash64(const void* data, std::size_t size);
    uint64_t Hash64(const std::string& string);
    std::string Hash2String(uint64_t hash);
}
