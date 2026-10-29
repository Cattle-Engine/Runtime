#pragma once

#include <cstdint>
#include <ios>
#include <istream>
#include <string>
#include <bit>

namespace CECommon::FS {
    class BinaryReader {
    public:
        explicit BinaryReader(std::istream& stream)
            : mStream(stream) {}

        bool Read(void* destination, size_t size);

        bool ReadU8(uint8_t& value);
        bool ReadU32(uint32_t& value);
        bool ReadU64(uint64_t& value);

        bool ReadI64(int64_t& value);

        bool ReadString(std::string& value, size_t size);

        bool ReadU16(uint16_t& v) {
            uint8_t lo, hi;
            if (!ReadU8(lo) || !ReadU8(hi)) return false;
            v = static_cast<uint16_t>(lo | (hi << 8));
            return true;
        }
        bool ReadI32(int32_t& v) { uint32_t u; if (!ReadU32(u)) return false; v = static_cast<int32_t>(u); return true; }
        bool ReadBool(bool& v)   { uint8_t u;  if (!ReadU8(u))  return false; v = u != 0; return true; }
        bool ReadF32(float& v)   { uint32_t u; if (!ReadU32(u)) return false; v = std::bit_cast<float>(u);  return true; }
        bool ReadF64(double& v)  { uint64_t u; if (!ReadU64(u)) return false; v = std::bit_cast<double>(u); return true; }

        bool ReadSizedString(std::string& v, uint32_t maxLength = 1u << 24) {
            uint32_t len;
            if (!ReadU32(len) || len > maxLength) return false;
            return ReadString(v, len);
        }

        bool AtEnd() { return mStream.peek() == std::char_traits<char>::eof(); }

        bool Seek(uint64_t offset, std::ios::seekdir seek_dir);
        uint64_t Tell() const;

    private:
        std::istream& mStream;
    };
}