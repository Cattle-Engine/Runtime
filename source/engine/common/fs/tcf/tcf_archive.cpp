#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "engine/common/fs/binary_reader.hpp"
#include "engine/common/fs/tcf/tcf.hpp"
#include "engine/common/tracelog.hpp"

#include <lz4.h>
#include <zstd.h>

#define TCF_READ_OR_ERROR(expr, message)                                                                               \
    if (!(expr)) {                                                                                                     \
        CE_LOG(LogLevel::Error, "[TCFArchive] {}", message);                                                           \
        return false;                                                                                                  \
    }

namespace CE::Common::FS::TCF {
    namespace {
        constexpr uint64_t kChunkHeaderSize = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint64_t) +
                                              sizeof(uint64_t) + sizeof(uint64_t);

        constexpr uint64_t kDirectoryContentSize = sizeof(uint8_t) + sizeof(uint64_t);

        std::string_view NormalizeArchivePath(std::string_view path) {
            if (path == "/") {
                return {};
            }

            if (!path.empty() && path.front() == '/') {
                path.remove_prefix(1);
            }

            return path;
        }

        uint32_t Crc32(const uint8_t* data, size_t size) {
            static const std::array<uint32_t, 256> table = [] {
                std::array<uint32_t, 256> result{};

                for (uint32_t i = 0; i < 256; ++i) {
                    uint32_t crc = i;

                    for (int bit = 0; bit < 8; ++bit) {
                        crc = (crc & 1) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
                    }

                    result[i] = crc;
                }

                return result;
            }();

            uint32_t crc = 0xFFFFFFFFu;

            for (size_t i = 0; i < size; ++i) {
                crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
            }

            return crc ^ 0xFFFFFFFFu;
        }

        bool ValidateName(std::string_view name, uint64_t max_size, std::string_view object_description) {
            if (name.empty()) {
                CE_LOG(LogLevel::Error, "[TCFArchive] {} has an empty name!", object_description);
                return false;
            }

            if (name.size() > max_size) {
                CE_LOG(LogLevel::Error, "[TCFArchive] {} name is too large. Maximum {}, got {}", object_description,
                       max_size, name.size());
                return false;
            }

            if (name == "." || name == "..") {
                CE_LOG(LogLevel::Error, "[TCFArchive] {} has forbidden name '{}'", object_description, name);
                return false;
            }

            for (char c : name) {
                if (c == '/' || c == '\\' || c == '\0') {
                    CE_LOG(LogLevel::Error, "[TCFArchive] {} contains an invalid character!", object_description);
                    return false;
                }

                if (static_cast<unsigned char>(c) > 0x7F) {
                    CE_LOG(LogLevel::Error, "[TCFArchive] {} contains a non-ASCII character!", object_description);
                    return false;
                }
            }

            return true;
        }

        bool ValidatePath(std::string_view path) {
            path = NormalizeArchivePath(path);

            if (path == "/") {
                return true;
            }

            if (path.empty()) {
                return false;
            }

            if (path.front() == '/' || path.back() == '/') {
                return false;
            }

            size_t component_start = 0;

            while (component_start < path.size()) {
                const size_t separator = path.find('/', component_start);

                const size_t component_end =
                    separator == std::string_view::npos ? path.size() : separator;

                const std::string_view component(
                    path.data() + component_start,
                    component_end - component_start);

                if (component.empty()) {
                    return false;
                }

                if (component == "." || component == "..") {
                    return false;
                }

                for (char c : component) {
                    if (c == '\\' || c == '\0') {
                        return false;
                    }

                    if (static_cast<unsigned char>(c) > 0x7F) {
                        return false;
                    }
                }

                if (separator == std::string_view::npos) {
                    break;
                }

                component_start = separator + 1;
            }

            return true;
        }
    } // namespace

    TCFArchive::TCFArchive(const fs::path& path) {
        if (!fs::exists(path)) {
            throw std::runtime_error("Archive path does not exist!");
        }

        mFile.open(path, std::ios::in | std::ios::binary);

        if (!mFile.is_open()) {
            throw std::runtime_error("Failed to open tcf archive");
        }

        mArchiveSize = fs::file_size(path);

        BinaryReader reader(mFile);

        if (!ReadHeader(reader)) {
            throw std::runtime_error("Failed to read header!");
        }

        if (!ReadFileInfo(reader)) {
            throw std::runtime_error("Failed to parse file info");
        }

        if (!ReadDirectoryInfo(reader)) {
            throw std::runtime_error("Failed to parse directory info");
        }
    }

    bool TCFArchive::ReadHeader(BinaryReader& reader) {
        if (mArchiveSize < kHeaderSize) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Archive is smaller than the TCF header. "
                   "Size {}, expected at least {}",
                   mArchiveSize, kHeaderSize);
            return false;
        }

        /*
            Fixed-size TCF header:

                0x00  3   magic "TCF"
                0x03  1   version
                0x04  1   endianness
                0x05  3   reserved
                0x08  8   file_info_offset
                0x10  8   file_count
                0x18  8   data_offset
                0x20  4   header crc32
                0x24  8   directory_table_offset
                0x2C  8   directory_count
                0x34  76  reserved
        */

        std::array<uint8_t, kHeaderSize> header_bytes{};

        TCF_READ_OR_ERROR(reader.Read(header_bytes.data(), header_bytes.size()), "Failed to read TCF header bytes!");

        if (std::string_view(reinterpret_cast<const char*>(header_bytes.data()), kTCFMagic.size()) != kTCFMagic) {
            CE_LOG(LogLevel::Error, "[TCFArchive] TCF magic does not match, expected 'TCF' got '{}'",
                   std::string_view(reinterpret_cast<const char*>(header_bytes.data()), kTCFMagic.size()));
            return false;
        }

        mHeaderInfo.version = header_bytes[3];

        if (mHeaderInfo.version != kVersion) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] TCF archive version is not supported. "
                   "Expected: {}, got {}",
                   kVersion, mHeaderInfo.version);
            return false;
        }

        const uint8_t endianness = header_bytes[4];

        if (static_cast<Endianness>(endianness) != kEndianness) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Endianness set in TCF is not supported!");
            return false;
        }

        for (size_t i = 5; i < 8; ++i) {
            if (header_bytes[i] != 0) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Header reserved bytes must be zero!");
                return false;
            }
        }

        mHeaderInfo.file_info_offset = ReadLEU64(header_bytes.data() + 0x08);

        mHeaderInfo.file_count = ReadLEU64(header_bytes.data() + 0x10);

        mHeaderInfo.data_offset = ReadLEU64(header_bytes.data() + 0x18);

        mHeaderInfo.crc = ReadLEU32(header_bytes.data() + 0x20);

        mHeaderInfo.directory_table_offset = ReadLEU64(header_bytes.data() + 0x24);

        mHeaderInfo.directory_count = ReadLEU64(header_bytes.data() + 0x2C);

        for (size_t i = 0x34; i < kHeaderSize; ++i) {
            if (header_bytes[i] != 0) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Header reserved bytes must be zero!");
                return false;
            }
        }

        /*
            The header CRC is computed over the first kHeaderCrcLen bytes
            of the header, which is everything before the CRC field.
        */
        const uint32_t computed_crc = Crc32(header_bytes.data(), kHeaderCrcLen);

        if (computed_crc != mHeaderInfo.crc) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Header CRC32 mismatch. Expected {}, got {}", mHeaderInfo.crc,
                   computed_crc);
            return false;
        }

        if (mHeaderInfo.data_offset < kHeaderSize) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Data offset {} is inside the header!", mHeaderInfo.data_offset);
            return false;
        }

        if (mHeaderInfo.data_offset > mArchiveSize) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Data offset {} is outside the archive. "
                   "Archive size is {}",
                   mHeaderInfo.data_offset, mArchiveSize);
            return false;
        }

        if (mHeaderInfo.file_info_offset < mHeaderInfo.data_offset) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] File info offset {} is before the data region "
                   "which starts at {}!",
                   mHeaderInfo.file_info_offset, mHeaderInfo.data_offset);
            return false;
        }

        if (mHeaderInfo.file_info_offset > mArchiveSize) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] File info offset {} is outside the archive. "
                   "Archive size is {}",
                   mHeaderInfo.file_info_offset, mArchiveSize);
            return false;
        }

        if (mHeaderInfo.directory_table_offset < mHeaderInfo.file_info_offset) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Directory table offset {} is before the "
                   "file info region at {}!",
                   mHeaderInfo.directory_table_offset, mHeaderInfo.file_info_offset);
            return false;
        }

        if (mHeaderInfo.directory_table_offset > mArchiveSize) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Directory table offset {} is outside the "
                   "archive. Archive size is {}",
                   mHeaderInfo.directory_table_offset, mArchiveSize);
            return false;
        }

        if (mHeaderInfo.directory_count == 0) {
            CE_LOG(LogLevel::Error, "[TCFArchive] TCF archive must contain a root directory!");
            return false;
        }

        if (mHeaderInfo.file_count > kMaxFileCount) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Max file count exceeded. Max {}, got {}", kMaxFileCount,
                   mHeaderInfo.file_count);
            return false;
        }

        if (mHeaderInfo.directory_count > kMaxFileCount) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Max directory count exceeded. Max {}, got {}", kMaxFileCount,
                   mHeaderInfo.directory_count);
            return false;
        }

        /*
            A file record has a minimum size of:

                uint64 file_id
                int64  date_modified
                uint32 name_size
                uint64 parent_directory
                uint64 chunk_count
                uint64 chunks_start_offset

            The name itself may be zero bytes here because the size check
            is only being used to make sure the record count can physically
            fit in the region.
        */
        if (mHeaderInfo.file_count > 0) {
            constexpr uint64_t kMinimumFileInfoSize = sizeof(uint64_t) + sizeof(int64_t) + sizeof(uint32_t) +
                                                      sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t);

            const uint64_t file_info_size = mHeaderInfo.directory_table_offset - mHeaderInfo.file_info_offset;

            if (file_info_size < kMinimumFileInfoSize ||
                mHeaderInfo.file_count > file_info_size / kMinimumFileInfoSize) {
                CE_LOG(LogLevel::Error, "[TCFArchive] File count cannot fit inside the "
                                        "file info region!");
                return false;
            }
        }

        /*
            A directory record has a minimum size of:

                uint64 id
                int64  date_modified
                uint32 name_size
                uint64 parent
                uint64 content_end

            Directory contents are additional variable-sized records.
        */
        if (mHeaderInfo.directory_count > 0) {
            constexpr uint64_t kMinimumDirectoryInfoSize =
                sizeof(uint64_t) + sizeof(int64_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);

            const uint64_t directory_region_size = mArchiveSize - mHeaderInfo.directory_table_offset;

            if (directory_region_size < kMinimumDirectoryInfoSize ||
                mHeaderInfo.directory_count > directory_region_size / kMinimumDirectoryInfoSize) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Directory count cannot fit inside the "
                                        "directory table region!");
                return false;
            }
        }

        return true;
    }

    bool TCFArchive::ReadFileInfo(BinaryReader& reader) {
        if (!reader.Seek(mHeaderInfo.file_info_offset, std::ios::beg)) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Failed to seek to file info at offset {}!",
                mHeaderInfo.file_info_offset);
            return false;
        }

        mFiles.clear();
        mFiles.reserve(mHeaderInfo.file_count);

        struct ChunkBlockRange {
            uint64_t start;
            uint64_t end;
        };

        std::vector<ChunkBlockRange> chunk_blocks;
        chunk_blocks.reserve(mHeaderInfo.file_count);

        uint64_t file_info_cursor = 0;

        for (uint64_t expected_file_id = 0; expected_file_id < mHeaderInfo.file_count; ++expected_file_id) {
            /*
                Chunk traversal moves the reader into the chunk-data region.
                Re-seek to the start of this file's record before parsing it.
            */
            if (!reader.Seek(mHeaderInfo.file_info_offset + file_info_cursor, std::ios::beg)) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Failed to seek to file info record {}!",
                    expected_file_id);
                return false;
            }

            FileInfo info;

            uint64_t stored_file_id = 0;

            TCF_READ_OR_ERROR(reader.ReadU64(stored_file_id), "Failed to read file ID!");

            if (stored_file_id != expected_file_id) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Expected file ID {}, got {}",
                    expected_file_id, stored_file_id);
                return false;
            }

            info.id = stored_file_id;

            TCF_READ_OR_ERROR(reader.ReadI64(info.date_modified),
                            "Failed to read file modification date!");

            uint32_t name_size = 0;

            TCF_READ_OR_ERROR(reader.ReadU32(name_size),
                            "Failed to read file name size!");

            if (name_size == 0) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File {} has an empty name!",
                    expected_file_id);
                return false;
            }

            if (name_size > kMaxNameSize) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File {} name is too large. "
                    "Maximum {}, got {}",
                    expected_file_id, kMaxNameSize, name_size);
                return false;
            }

            info.name.resize(name_size);

            TCF_READ_OR_ERROR(reader.Read(info.name.data(), name_size),
                            "Failed to read file name!");

            if (!ValidateName(info.name, kMaxNameSize, "File")) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File {} has invalid name '{}'",
                    expected_file_id, info.name);
                return false;
            }

            TCF_READ_OR_ERROR(reader.ReadU64(info.parent_directory),
                            "Failed to read file parent directory!");

            if (info.parent_directory >= mHeaderInfo.directory_count) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File {} references invalid parent "
                    "directory {}",
                    expected_file_id, info.parent_directory);
                return false;
            }

            TCF_READ_OR_ERROR(reader.ReadU64(info.chunk_count),
                            "Failed to read chunk count!");

            if (info.chunk_count > kMaxChunksPerFile) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File '{}' has too many chunks. "
                    "Maximum {}, got {}",
                    info.name, kMaxChunksPerFile, info.chunk_count);
                return false;
            }

            TCF_READ_OR_ERROR(reader.ReadU64(info.chunk_block_offset),
                            "Failed to read chunk block offset!");

            /*
                Remember where the next file record starts.

                This must be done before traversing the chunk block because
                chunk parsing moves the reader into the data region.
            */
            file_info_cursor = reader.Tell() - mHeaderInfo.file_info_offset;

            /*
                An empty file has no chunk block. The offset is therefore
                not used for parsing it.
            */
            if (info.chunk_count == 0) {
                mFiles.push_back(std::move(info));
                continue;
            }

            /*
                For a non-empty file, the chunk block must live entirely
                between the data region and file-info region.
            */
            if (info.chunk_block_offset < mHeaderInfo.data_offset) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File '{}' chunk block starts before "
                    "the data region!",
                    info.name);
                return false;
            }

            if (info.chunk_block_offset >= mHeaderInfo.file_info_offset) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File '{}' chunk block starts inside "
                    "or after the file info region!",
                    info.name);
                return false;
            }

            const uint64_t available_chunk_region =
                mHeaderInfo.file_info_offset - info.chunk_block_offset;

            if (info.chunk_count > available_chunk_region / (kChunkHeaderSize + 1)) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File '{}' has too many chunks to fit "
                    "inside its chunk region!",
                    info.name);
                return false;
            }

            if (!reader.Seek(info.chunk_block_offset, std::ios::beg)) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] Failed to seek to chunk block for "
                    "file '{}'!",
                    info.name);
                return false;
            }

            info.chunks.resize(info.chunk_count);

            uint64_t expected_offset = info.chunk_block_offset;

            for (uint64_t expected_chunk_id = 0;
                expected_chunk_id < info.chunk_count;
                ++expected_chunk_id) {
                if (!reader.Seek(expected_offset, std::ios::beg)) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Failed to seek to chunk {} "
                        "of file '{}'!",
                        expected_chunk_id, info.name);
                    return false;
                }

                uint64_t stored_chunk_id = 0;

                TCF_READ_OR_ERROR(reader.ReadU64(stored_chunk_id),
                                "Failed to read chunk ID!");

                if (stored_chunk_id != expected_chunk_id) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] File '{}' expected chunk ID {}, "
                        "got {}",
                        info.name, expected_chunk_id, stored_chunk_id);
                    return false;
                }

                auto& chunk = info.chunks[expected_chunk_id];

                TCF_READ_OR_ERROR(reader.ReadU32(chunk.crc32),
                                "Failed to read chunk CRC32!");

                uint8_t compression = 0;

                TCF_READ_OR_ERROR(reader.ReadU8(compression),
                                "Failed to read chunk compression type!");

                if (compression > static_cast<uint8_t>(CompressionType::None)) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Invalid compression type {} "
                        "for file '{}'",
                        compression, info.name);
                    return false;
                }

                chunk.compression = static_cast<CompressionType>(compression);

                TCF_READ_OR_ERROR(reader.ReadU64(chunk.compressed_size),
                                "Failed to read chunk compressed size!");

                TCF_READ_OR_ERROR(reader.ReadU64(chunk.uncompressed_size),
                                "Failed to read chunk uncompressed size!");

                TCF_READ_OR_ERROR(reader.ReadU64(chunk.offset),
                                "Failed to read chunk data end offset!");

                /*
                    reader is now immediately after the chunk header.
                */
                const uint64_t data_start =
                    expected_offset + kChunkHeaderSize;

                if (chunk.offset < data_start) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an "
                        "end offset before its data begins!",
                        expected_chunk_id, info.name);
                    return false;
                }

                if (chunk.offset >= mHeaderInfo.file_info_offset) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' extends "
                        "into the file info region!",
                        expected_chunk_id, info.name);
                    return false;
                }

                const uint64_t physical_compressed_size =
                    chunk.offset - data_start;

                if (chunk.compressed_size != physical_compressed_size) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an "
                        "invalid compressed size. Metadata says {}, "
                        "physical size is {}",
                        expected_chunk_id,
                        info.name,
                        chunk.compressed_size,
                        physical_compressed_size);
                    return false;
                }

                if (chunk.uncompressed_size > kFileChunkSize) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an "
                        "uncompressed size larger than the maximum "
                        "chunk size. Maximum {}, got {}",
                        expected_chunk_id,
                        info.name,
                        kFileChunkSize,
                        chunk.uncompressed_size);
                    return false;
                }

                if (chunk.compression == CompressionType::None &&
                    chunk.compressed_size != chunk.uncompressed_size) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Uncompressed chunk {} in file '{}' "
                        "has different compressed and uncompressed sizes!",
                        expected_chunk_id, info.name);
                    return false;
                }

                /*
                    data_end_offset points to the terminator byte.
                */
                if (!reader.Seek(chunk.offset, std::ios::beg)) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Failed to seek to chunk terminator!");
                    return false;
                }

                uint8_t terminator = 0;

                TCF_READ_OR_ERROR(reader.ReadU8(terminator),
                                "Failed to read chunk terminator!");

                if (terminator != 0) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' is missing "
                        "its 0x00 terminator!",
                        expected_chunk_id, info.name);
                    return false;
                }

                if (chunk.offset == UINT64_MAX) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an "
                        "invalid end offset!",
                        expected_chunk_id, info.name);
                    return false;
                }

                expected_offset = chunk.offset + 1;

                if (expected_offset > mHeaderInfo.file_info_offset) {
                    CE_LOG(LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' extends "
                        "beyond the chunk data region!",
                        expected_chunk_id, info.name);
                    return false;
                }
            }

            chunk_blocks.push_back({info.chunk_block_offset, expected_offset});

            mFiles.push_back(std::move(info));
        }

        /*
            File metadata must end exactly where the directory table starts.
        */
        if (!reader.Seek(mHeaderInfo.directory_table_offset, std::ios::beg)) {
            CE_LOG(LogLevel::Error,
                "[TCFArchive] Failed to seek to the directory table!");
            return false;
        }

        /*
            Make sure different files don't claim overlapping chunk blocks.
        */
        std::sort(chunk_blocks.begin(), chunk_blocks.end(),
                [](const ChunkBlockRange& a, const ChunkBlockRange& b) {
                    return a.start < b.start;
                });

        for (size_t i = 1; i < chunk_blocks.size(); ++i) {
            if (chunk_blocks[i].start < chunk_blocks[i - 1].end) {
                CE_LOG(LogLevel::Error,
                    "[TCFArchive] File chunk blocks overlap!");
                return false;
            }
        }

        return true;
    }

    bool TCFArchive::ReadDirectoryInfo(BinaryReader& reader) {
        if (!reader.Seek(mHeaderInfo.directory_table_offset, std::ios::beg)) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Failed to seek to directory table at "
                   "offset {}!",
                   mHeaderInfo.directory_table_offset);
            return false;
        }

        uint64_t last_content_end = mHeaderInfo.directory_table_offset;

        mDirectories.clear();
        mDirectories.reserve(mHeaderInfo.directory_count);

        for (uint64_t expected_directory_id = 0; expected_directory_id < mHeaderInfo.directory_count;
             ++expected_directory_id) {
            DirectoryInfo info;

            uint64_t stored_directory_id = 0;

            TCF_READ_OR_ERROR(reader.ReadU64(stored_directory_id), "Failed to read directory ID!");

            if (stored_directory_id != expected_directory_id) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Expected directory ID {}, got {}", expected_directory_id,
                       stored_directory_id);
                return false;
            }

            info.id = stored_directory_id;

            TCF_READ_OR_ERROR(reader.ReadI64(info.date_modified), "Failed to read directory modification date!");

            uint32_t name_size = 0;

            TCF_READ_OR_ERROR(reader.ReadU32(name_size), "Failed to read directory name size!");

            if (name_size > kMaxNameSize) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} name is too large. "
                       "Maximum {}, got {}",
                       expected_directory_id, kMaxNameSize, name_size);
                return false;
            }

            /*
                The root directory is represented by directory ID 0 and
                has an empty name.

                All other directories must have a normal component name.
            */
            if (name_size == 0 && expected_directory_id != 0) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Non-root directory {} has an empty name!", expected_directory_id);
                return false;
            }

            info.name.resize(name_size);

            if (name_size > 0) {
                TCF_READ_OR_ERROR(reader.Read(info.name.data(), name_size), "Failed to read directory name!");
            }

            if (expected_directory_id == 0) {
                if (!info.name.empty()) {
                    CE_LOG(LogLevel::Error, "[TCFArchive] Root directory must have an empty name!");
                    return false;
                }
            } else {
                if (!ValidateName(info.name, kMaxNameSize, "Directory")) {
                    CE_LOG(LogLevel::Error, "[TCFArchive] Directory {} has invalid name '{}'", expected_directory_id,
                           info.name);
                    return false;
                }
            }

            TCF_READ_OR_ERROR(reader.ReadU64(info.parent), "Failed to read directory parent!");

            if (expected_directory_id == 0) {
                if (info.parent != 0) {
                    CE_LOG(LogLevel::Error, "[TCFArchive] Root directory must have itself "
                                            "as its parent!");
                    return false;
                }
            } else if (info.parent >= mHeaderInfo.directory_count) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} references invalid "
                       "parent directory {}",
                       expected_directory_id, info.parent);
                return false;
            } else if (info.parent == expected_directory_id) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Directory {} cannot be its own parent!", expected_directory_id);
                return false;
            }

            uint64_t content_end = 0;

            TCF_READ_OR_ERROR(reader.ReadU64(content_end), "Failed to read directory content end!");

            /*
                At this point reader is positioned at the beginning of
                this directory's content block.
            */
            const uint64_t content_start =
                mHeaderInfo.directory_table_offset + 0; // overwritten below by seeking-relative position

            /*
                BinaryReader does not need to expose its current position
                here. We know the current position from the serialized
                record, so seek to content_end only after validating it
                against the archive bounds.

                The exact current position is reconstructed by reading
                the fixed fields above from the archive. To avoid relying
                on sizeof() or C++ struct padding, calculate it explicitly.
            */
            const uint64_t current_position = reader.Tell();

            if (content_end < current_position) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} has a content end offset "
                       "before the beginning of its content!",
                       expected_directory_id);
                return false;
            }

            if (content_end > mArchiveSize) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} content end {} is outside "
                       "the archive. Archive size is {}",
                       expected_directory_id, content_end, mArchiveSize);
                return false;
            }

            const uint64_t content_size = content_end - current_position;

            if (content_size % kDirectoryContentSize != 0) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} content block has an "
                       "invalid size of {} bytes!",
                       expected_directory_id, content_size);
                return false;
            }

            const uint64_t content_count = content_size / kDirectoryContentSize;

            const uint64_t maximum_content_count = mHeaderInfo.file_count + mHeaderInfo.directory_count;

            if (content_count > maximum_content_count) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} contains {} entries, "
                       "which exceeds the maximum possible number of "
                       "archive objects!",
                       expected_directory_id, content_count);
                return false;
            }

            info.contents.reserve(static_cast<size_t>(content_count));

            for (uint64_t content_index = 0; content_index < content_count; ++content_index) {
                DirectoryContent content;

                uint8_t type = 0;

                TCF_READ_OR_ERROR(reader.ReadU8(type), "Failed to read directory content type!");

                if (type > static_cast<uint8_t>(DirectoryContentType::File)) {
                    CE_LOG(LogLevel::Error,
                           "[TCFArchive] Directory {} contains an "
                           "invalid content type {}",
                           expected_directory_id, type);
                    return false;
                }

                content.type = static_cast<DirectoryContentType>(type);

                TCF_READ_OR_ERROR(reader.ReadU64(content.id), "Failed to read directory content ID!");

                if (content.type == DirectoryContentType::Directory) {
                    if (content.id >= mHeaderInfo.directory_count) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains a "
                               "reference to invalid directory {}",
                               expected_directory_id, content.id);
                        return false;
                    }

                    if (content.id == 0) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains the "
                               "root directory as a child!",
                               expected_directory_id);
                        return false;
                    }

                    if (content.id == expected_directory_id) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains itself "
                               "as a child!",
                               expected_directory_id);
                        return false;
                    }
                } else {
                    if (content.id >= mHeaderInfo.file_count) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains a "
                               "reference to invalid file {}",
                               expected_directory_id, content.id);
                        return false;
                    }
                }

                last_content_end = content_end;
                info.contents.push_back(content);
            }

            /*
                content_end must be exactly where the next directory
                record starts.
            */
            if (!reader.Seek(content_end, std::ios::beg)) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Failed to seek to the end of "
                       "directory {} content!",
                       expected_directory_id);
                return false;
            }

            mDirectories.push_back(std::move(info));
        }

        /*
            The directory table is the final region of the archive.
            Therefore, after reading the final directory, we should be
            exactly at EOF.
        */
        if (!reader.Seek(mHeaderInfo.directory_table_offset, std::ios::beg)) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Failed to re-seek to directory table!");
            return false;
        }

        /*
            Re-walk the table from the parsed in-memory representation
            to validate parent/child relationships and names.

            This also guarantees:
              - every file appears exactly once
              - every non-root directory appears exactly once
              - file parent metadata matches its directory entry
              - directory parent metadata matches its directory entry
              - sibling names are unique
        */

        std::vector<uint32_t> file_reference_count(mHeaderInfo.file_count, 0);

        std::vector<uint32_t> directory_reference_count(mHeaderInfo.directory_count, 0);

        for (const DirectoryInfo& directory : mDirectories) {
            std::unordered_set<std::string> child_names;
            child_names.reserve(directory.contents.size());

            for (const DirectoryContent& content : directory.contents) {
                std::string child_name;

                if (content.type == DirectoryContentType::Directory) {
                    const DirectoryInfo& child = mDirectories[content.id];

                    child_name = child.name;

                    if (child.parent != directory.id) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains directory {}, "
                               "but its parent is {}!",
                               directory.id, child.id, child.parent);
                        return false;
                    }

                    if (++directory_reference_count[content.id] != 1) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} is referenced more "
                               "than once!",
                               content.id);
                        return false;
                    }
                } else {
                    const FileInfo& file = mFiles[content.id];

                    child_name = file.name;

                    if (file.parent_directory != directory.id) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] Directory {} contains file {}, "
                               "but the file's parent is {}!",
                               directory.id, file.id, file.parent_directory);
                        return false;
                    }

                    if (++file_reference_count[content.id] != 1) {
                        CE_LOG(LogLevel::Error,
                               "[TCFArchive] File {} is referenced more "
                               "than once!",
                               content.id);
                        return false;
                    }
                }

                if (!child_names.insert(std::move(child_name)).second) {
                    CE_LOG(LogLevel::Error,
                           "[TCFArchive] Directory {} contains duplicate "
                           "child name!",
                           directory.id);
                    return false;
                }
            }
        }

        /*
            Every file must be reachable from exactly one directory.
        */
        for (uint64_t file_id = 0; file_id < mHeaderInfo.file_count; ++file_id) {
            if (file_reference_count[file_id] != 1) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] File {} is referenced {} times. "
                       "Every file must belong to exactly one directory!",
                       file_id, file_reference_count[file_id]);
                return false;
            }
        }

        /*
            Every non-root directory must be reachable from exactly one
            parent directory. The root is the only directory that does
            not appear as a child.
        */
        for (uint64_t directory_id = 1; directory_id < mHeaderInfo.directory_count; ++directory_id) {
            if (directory_reference_count[directory_id] != 1) {
                CE_LOG(LogLevel::Error,
                       "[TCFArchive] Directory {} is referenced {} times. "
                       "Every non-root directory must belong to exactly "
                       "one parent directory!",
                       directory_id, directory_reference_count[directory_id]);
                return false;
            }
        }

        /*
            Detect parent cycles.

            Directory IDs do not have to be ordered according to their
            hierarchy, so we cannot simply require parent < child.
        */
        std::vector<uint8_t> directory_state(mHeaderInfo.directory_count, 0);

        directory_state[0] = 2;

        for (uint64_t start = 1; start < mHeaderInfo.directory_count; ++start) {
            if (directory_state[start] == 2) {
                continue;
            }

            std::vector<uint64_t> path;
            uint64_t current = start;

            while (current != 0 && directory_state[current] == 0) {
                directory_state[current] = 1;
                path.push_back(current);
                current = mDirectories[current].parent;
            }

            if (current != 0 && directory_state[current] == 1) {
                CE_LOG(LogLevel::Error, "[TCFArchive] Directory hierarchy contains a cycle!");
                return false;
            }

            for (uint64_t directory_id : path) {
                directory_state[directory_id] = 2;
            }
        }

        if (last_content_end != mArchiveSize) {
            CE_LOG(LogLevel::Error,
                   "[TCFArchive] Directory table does not end at the end "
                   "of the archive. Directory table ended at {}, archive "
                   "size is {}",
                   last_content_end, mArchiveSize);
            return false;
        }

        return true;
    }

    bool TCFArchive::FileExists(const std::string& path) const {
        FileId file_id = 0;
        return ResolvePath(path, DirectoryContentType::File, file_id);
    }

    bool TCFArchive::GetFileSize(const std::string& path, uint64_t& size) const {
        if (!ValidatePath(path)) {
            return false;
        }

        const std::string_view archive_path = NormalizeArchivePath(path);

        uint64_t current_directory = 0;
        const FileInfo* found_file = nullptr;

        size_t component_start = 0;

        while (component_start < archive_path.size()) {
            const size_t separator = archive_path.find('/', component_start);

            const size_t component_end = separator == std::string_view::npos ? archive_path.size() : separator;

            const std::string_view component = archive_path.substr(component_start, component_end - component_start);

            const bool last_component = separator == std::string::npos;

            const DirectoryInfo& directory = mDirectories[current_directory];

            bool found = false;

            for (const DirectoryContent& content : directory.contents) {
                if (content.type == DirectoryContentType::Directory) {
                    const DirectoryInfo& child = mDirectories[content.id];

                    if (child.name != component) {
                        continue;
                    }

                    if (last_component) {
                        return false;
                    }

                    current_directory = child.id;
                    found = true;
                    break;
                }

                if (last_component) {
                    const FileInfo& file = mFiles[content.id];

                    if (file.name == component) {
                        found_file = &file;
                        break;
                    }
                }
            }

            if (found_file != nullptr) {
                break;
            }

            if (!found && !last_component) {
                return false;
            }

            if (separator == std::string::npos) {
                break;
            }

            component_start = separator + 1;
        }

        if (found_file == nullptr) {
            return false;
        }

        size = 0;

        for (const auto& chunk : found_file->chunks) {
            if (chunk.uncompressed_size > std::numeric_limits<uint64_t>::max() - size) {
                CE_LOG(LogLevel::Error, "[TCFArchive] File '{}' size overflow!", path);
                return false;
            }

            size += chunk.uncompressed_size;
        }

        return true;
    }

    bool TCFArchive::ResolvePath(const std::string& path,
                                DirectoryContentType type,
                                uint64_t& id) const
    {
        if (path.empty() || path == "/") {
            if (type == DirectoryContentType::Directory) {
                id = 0;
                return true;
            }

            return false;
        }

        const std::string_view archive_path = NormalizeArchivePath(path);

        if (!ValidatePath(archive_path)) {
            return false;
        }

        uint64_t current_directory = 0;

        size_t component_start = 0;

        while (component_start < archive_path.size()) {
            const size_t separator = archive_path.find('/', component_start);

            const size_t component_end =
                separator == std::string_view::npos
                    ? archive_path.size()
                    : separator;

            const std::string_view component =
                archive_path.substr(component_start,
                                    component_end - component_start);

            const bool is_final_component =
                separator == std::string_view::npos;

            const DirectoryInfo& directory = mDirectories[current_directory];

            bool found = false;

            for (const DirectoryContent& content : directory.contents) {
                if (content.type == DirectoryContentType::Directory) {
                    const DirectoryInfo& child = mDirectories[content.id];

                    if (child.name != component) {
                        continue;
                    }

                    if (is_final_component) {
                        if (type != DirectoryContentType::Directory) {
                            return false;
                        }

                        id = child.id;
                        return true;
                    }

                    current_directory = child.id;
                    found = true;
                    break;
                }

                if (content.type == DirectoryContentType::File) {
                    if (!is_final_component) {
                        continue;
                    }

                    const FileInfo& file = mFiles[content.id];

                    if (file.name != component) {
                        continue;
                    }

                    if (type != DirectoryContentType::File) {
                        return false;
                    }

                    id = file.id;
                    return true;
                }
            }

            if (!found && !is_final_component) {
                return false;
            }

            if (is_final_component) {
                return false;
            }

            component_start = component_end + 1;
        }

        return false;
    }

    bool TCFArchive::FindFile(const std::string& path, FileId& file_id) const {
        return ResolvePath(path, DirectoryContentType::File, file_id);
    }

    bool TCFArchive::DirExists(const std::string& path) const {
        DirectoryId directory_id = 0;
        return ResolvePath(path, DirectoryContentType::Directory, directory_id);
    }

    TCFDirectoryContents TCFArchive::ListDirectory(const std::string& path) const {
        DirectoryId dir_id = 0;
        if(!ResolvePath(path, DirectoryContentType::Directory, dir_id)) {
            return {};
        }

        TCFDirectoryContents out;
        for (auto& content : mDirectories[dir_id].contents) {
            if (content.type == DirectoryContentType::File) {
                out.files.push_back({mFiles[content.id].name, mFiles[content.id].date_modified});
            } else {
                out.directories.push_back({mDirectories[content.id].name, mDirectories[content.id].date_modified});
            }
        }

        return out;
    }

    bool TCFArchive::GetDirLastModified(const std::string& path, int64_t& timestamp_ms) {
        DirectoryId dir = 0;
        if (!ResolvePath(path, DirectoryContentType::Directory, dir)) {
            CE_LOG(LogLevel::Error, "[TCFArchive] Could not find directory: {}", path);
            return false;
        }

        timestamp_ms = mDirectories[dir].date_modified;
        return true;
    }
} // namespace CE::Common::FS::TCF