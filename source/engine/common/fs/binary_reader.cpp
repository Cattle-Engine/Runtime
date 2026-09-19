#include "engine/common/fs/binary_reader.hpp"

namespace CE::Common::FS {
    bool BinaryReader::Read(void* destination, size_t size) {
        mStream.read(
            static_cast<char*>(destination),
            static_cast<std::streamsize>(size)
        );

        return mStream.good();
    }

    bool BinaryReader::ReadU8(uint8_t& value) {
        return Read(&value, sizeof(value));
    }

    bool BinaryReader::ReadU64(uint64_t& value) {
        uint8_t bytes[8];

        if (!Read(bytes, sizeof(bytes))) {
            return false;
        }

        value =
            (static_cast<uint64_t>(bytes[0]) << 0) |
            (static_cast<uint64_t>(bytes[1]) << 8) |
            (static_cast<uint64_t>(bytes[2]) << 16) |
            (static_cast<uint64_t>(bytes[3]) << 24) |
            (static_cast<uint64_t>(bytes[4]) << 32) |
            (static_cast<uint64_t>(bytes[5]) << 40) |
            (static_cast<uint64_t>(bytes[6]) << 48) |
            (static_cast<uint64_t>(bytes[7]) << 56);

        return true;
    }

    bool BinaryReader::ReadU32(uint32_t& value) {
        uint8_t bytes[4];

        if (!Read(bytes, sizeof(bytes))) {
            return false;
        }

        value =
            (static_cast<uint32_t>(bytes[0]) << 0) |
            (static_cast<uint32_t>(bytes[1]) << 8) |
            (static_cast<uint32_t>(bytes[2]) << 16) |
            (static_cast<uint32_t>(bytes[3]) << 24);

        return true;
    }
    
    bool BinaryReader::Seek(uint64_t offset, std::ios::seekdir seek_dir) {
        mStream.seekg(static_cast<std::istream::off_type>(offset), seek_dir);
        return static_cast<bool>(mStream);
    }
}