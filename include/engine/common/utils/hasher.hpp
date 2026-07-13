#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace CE::Utils {
    class StreamingHasher {
        public:
            StreamingHasher();
            ~StreamingHasher();
            
            void AddData(const void* data, std::size_t size);
            void AddString(const std::string& string);
            template <typename T>
            void AddValue(T&& value) {
                AddData(&value, sizeof(value));
            }
            uint64_t Finalize();
        private:
            class Impl;
            std::unique_ptr<Impl> mImpl;
    };
    
    uint64_t Hash64(const void* data, std::size_t size);
    uint64_t Hash64(const std::string& string);
    std::string Hash2String(uint64_t hash);
}
