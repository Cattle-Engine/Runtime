#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "engine/common/fs/binary_reader.hpp"

namespace fs = std::filesystem;

namespace CE::Common::FS::TCF {
    class TCFFile;

    class TCFArchive {
      public:
        // May throw std::runtime if:
        // 1. the file does not exist
        // 2. it fails to parse the tcf archive
        TCFArchive(const fs::path& path);

        TCFFile OpenFile(const std::string& path);
        void CloseFile(TCFFile& file);
        bool FileExists(const std::string& path) const;
        bool GetFileSize(const std::string& path, uint64_t& size) const;

      private:
        friend class TCFFile;

        enum class CompressionType : uint8_t {
            Zstd = 0,
            LZ4 = 1,
            None = 2
        };

        enum class Endianness : uint8_t {
            Little = 0,
            Big = 1
        };

        static constexpr std::string_view kTCFMagic = "TCF";
        static constexpr uint8_t kVersion = 3;
        static constexpr Endianness kEndianness = Endianness::Little;
        static constexpr size_t kBufferSize = 64 * 1024;
        static constexpr uint64_t kHeaderSize = 128;
        static constexpr uint64_t kHeaderCrcLen = 0x20;
        static constexpr uint32_t kFileChunkSize = 256 * 1024;
        static constexpr uint64_t kMaxPathSize = 9096;
        static constexpr uint64_t kMaxChunksPerFile = 1'000'000;
        static constexpr uint64_t kMaxFileCount = 10'000'000;

        /*
            For file info on disk we do:
            [uint64_file_path_size][ascii_path][uint64_chunk_count][uint64_chunks_start_offset]

            Path rules:
                - '/' is the separator
                - '\' is forbidden
                - paths are case-sensitive
                - no leading '/'
                - no trailing '/'
                - no empty components
                - '.' and '..' are forbidden in file names
                - NUL is forbidden

            For the chunk on disk we have this info. Chunks are stored in a contiguous block
            [uint64_chunk_id][uint32_crc32][uint8_compression_type][compressed_size][uncompressed_size][uint64_data_end_offset][data][0x0]

            Chunk IDs are per file, and go up by one per chunk. Chunks a read until a chunk ID is the same as the chunk
           count in the file info. If a chunk with the next ID (or there's no 0 chunk) you will get an error.

            data_end_offset is an absolute archive offset immediately after the chunk's compressed data.
            uncompressed_size is for validation during decompression
        */
        struct FileInfo {
            struct Chunk {
                uint64_t offset;          // offset for the data end
                uint64_t compressed_size; // whenn compression is set to None, this is the same as uncompressed_size
                uint64_t uncompressed_size;
                CompressionType compression;
                uint32_t crc32;
            };

            uint64_t chunk_block_offset; // this is the absolute archive offset of the first byte of the first chunk
                                         // header for this file.
            uint64_t chunk_count;
            std::string file_path;
            std::vector<Chunk> chunks; // vector position is the id
        };

        /*
            The header is at 0x00, after this is the chunk data, and then the file metadata
            Offset  Size
            0x00    3    magic "TCF"
            0x03    1    version
            0x04    1    endianness
            0x05    3    reserved
            0x08    8    file_info_offset
            0x10    8    file_count
            0x18    8    data_offset
            0x20    4    header crc32
            0x24    92   reserved
        */
        struct HeaderInfo {
            uint8_t version;
            uint64_t file_info_offset; // file info is at the end of the chunk block
            uint64_t file_count;
            uint64_t data_offset;
            uint32_t crc;
        };

        std::fstream mFile;
        HeaderInfo mHeaderInfo;
        std::vector<FileInfo> mFiles;
        uint64_t mArchiveSize = 0;

        static uint32_t ReadLEU32(const uint8_t* p) {
            return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
                   (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
        }

        static uint64_t ReadLEU64(const uint8_t* p) {
            return static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8) |
                   (static_cast<uint64_t>(p[2]) << 16) | (static_cast<uint64_t>(p[3]) << 24) |
                   (static_cast<uint64_t>(p[4]) << 32) | (static_cast<uint64_t>(p[5]) << 40) |
                   (static_cast<uint64_t>(p[6]) << 48) | (static_cast<uint64_t>(p[7]) << 56);
        }

        bool ReadHeader(BinaryReader& reader);
        bool ReadFileInfo(BinaryReader& reader);
    };

    enum class SeekMode {
        Start,
        Current,
        End
    };

    class TCFFile {
      public:
        ~TCFFile();

        // if you use negative numbers this will move the offset backwards
        bool Seek(int64_t offset, SeekMode mode);
        uint64_t Tell() const;
        uint64_t Size() const;
        // moves the current offset forward by how many bytes size is
        // size is in bytes
        bool Read(void* destination, size_t size);
        bool IsValid() const;
        bool Eof() const;

      private:
        friend class TCFArchive;

        bool ReadChunk(const TCFArchive::FileInfo::Chunk& chunk, std::vector<uint8_t>& output);
        TCFFile(TCFArchive& archive, uint64_t end_offset) : mTCFArchive(archive), mEndOffset(end_offset) {}

        TCFArchive& mTCFArchive;
        uint64_t mCurrentOffset = 0;
        const uint64_t mEndOffset;
        size_t FileID = 0;
        bool mValid = false;
    };
} // namespace CE::Common::FS::TCF