#pragma once

#include <cstdint>
#include <ios>
#include <ostream>
#include <string>
#include <string_view>

namespace CECommon::FS {
    class BinaryWriter {
        public:
            explicit BinaryWriter(std::ostream& stream) : mOutput(stream) {}

            // Little-endian fixed-width integer writes.
            bool WriteU8(uint8_t value);
            bool WriteU16(uint16_t value);
            bool WriteU32(uint32_t value);
            bool WriteU64(uint64_t value);

            // Writes `size` raw bytes. No length prefix is emitted.
            bool WriteBytes(const void* data, size_t size);

            // Writes the raw contents of `value`. No length prefix is emitted.
            bool WriteString(std::string_view value);

            bool Flush();
            bool Good() const;
            uint64_t Tell();

        private:
            std::ostream& mOutput;
    };
}