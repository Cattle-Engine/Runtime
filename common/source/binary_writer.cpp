#include "ce_common/binary_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <ostream>
#include <string_view>

namespace CECommon::FS {
    namespace {
        // Appends `value` to `buffer` in little-endian byte order.
        template <typename T>
        void EncodeLittleEndian(T value, std::array<char, sizeof(T)>& buffer) {
            for (size_t i = 0; i < sizeof(T); ++i) {
                buffer[i] = static_cast<char>((value >> (i * 8)) & 0xFF);
            }
        }
    }

    bool BinaryWriter::WriteU8(uint8_t value) {
        const char byte = static_cast<char>(value);
        mOutput.write(&byte, 1);
        return mOutput.good();
    }

    bool BinaryWriter::WriteU16(uint16_t value) {
        std::array<char, sizeof(uint16_t)> buffer{};
        EncodeLittleEndian(value, buffer);
        mOutput.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        return mOutput.good();
    }

    bool BinaryWriter::WriteU32(uint32_t value) {
        std::array<char, sizeof(uint32_t)> buffer{};
        EncodeLittleEndian(value, buffer);
        mOutput.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        return mOutput.good();
    }

    bool BinaryWriter::WriteU64(uint64_t value) {
        std::array<char, sizeof(uint64_t)> buffer{};
        EncodeLittleEndian(value, buffer);
        mOutput.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        return mOutput.good();
    }

    bool BinaryWriter::WriteBytes(const void* data, size_t size) {
        if (size == 0) {
            return true;
        }
        if (data == nullptr) {
            return false;
        }
        mOutput.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        return mOutput.good();
    }

    bool BinaryWriter::WriteString(std::string_view value) {
        return WriteBytes(value.data(), value.size());
    }

    bool BinaryWriter::Flush() {
        mOutput.flush();
        return mOutput.good();
    }

    bool BinaryWriter::Good() const {
        return mOutput.good();
    }

    uint64_t BinaryWriter::Tell() {
        const std::streampos position = mOutput.tellp();
        if (position == std::streampos(-1)) {
            return 0;
        }
        return static_cast<uint64_t>(position);
    }
}