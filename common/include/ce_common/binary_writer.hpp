#pragma once

#include <cstdint>
#include <ios>
#include <ostream>
#include <string>
#include <string_view>
#include <bit>
#include <limits>

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

            bool WriteI32(int32_t v)  { return WriteU32(static_cast<uint32_t>(v)); }
            bool WriteI64(int64_t v)  { return WriteU64(static_cast<uint64_t>(v)); }
            bool WriteBool(bool v)    { return WriteU8(v ? 1 : 0); }
            bool WriteF32(float v)    { static_assert(sizeof(float) == 4);  return WriteU32(std::bit_cast<uint32_t>(v)); }
            bool WriteF64(double v)   { static_assert(sizeof(double) == 8); return WriteU64(std::bit_cast<uint64_t>(v)); }

            // u32 length prefix + raw bytes.
            bool WriteSizedString(std::string_view s) {
                if (s.size() > std::numeric_limits<uint32_t>::max()) return false;
                return WriteU32(static_cast<uint32_t>(s.size())) && WriteString(s);
            }

            bool Flush();
            bool Good() const;
            uint64_t Tell();

        private:
            std::ostream& mOutput;
    };
}