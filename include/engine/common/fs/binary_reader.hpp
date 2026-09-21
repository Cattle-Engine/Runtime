#pragma once

#include <cstdint>
#include <fstream>

namespace CE::Common::FS {
    class BinaryReader {
      public:
        explicit BinaryReader(std::istream& stream) : mStream(stream) {}

        bool Read(void* destination, size_t size);

        bool ReadU8(uint8_t& value);

        bool ReadU64(uint64_t& value);

        bool ReadU32(uint32_t& value);

        bool Seek(uint64_t offset, std::ios::seekdir seek_dir);

      private:
        std::istream& mStream;
    };
} // namespace CE::Common::FS