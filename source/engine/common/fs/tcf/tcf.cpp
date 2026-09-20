#include "engine/common/fs/tcf/tcf.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <limits>
#include <utility>
#include <cstring>

#include <zstd.h>
#include <lz4.h>

#include "engine/common/fs/binary_reader.hpp"
#include "engine/common/tracelog.hpp"

#define TCF_READ_OR_ERROR(expr, message) \
    if (!(expr)) { \
        CE_LOG(LogLevel::Error, "[TCFArchive] {}", message); \
        return false; \
    }

namespace CE::Common::FS::TCF {
    namespace {
        uint32_t Crc32(const uint8_t* data, size_t size) {
            static const std::array<uint32_t, 256> table = [] {
                std::array<uint32_t, 256> result{};

                for (uint32_t i = 0; i < 256; ++i) {
                    uint32_t crc = i;

                    for (int bit = 0; bit < 8; ++bit) {
                        crc = (crc & 1)
                            ? (0xEDB88320u ^ (crc >> 1))
                            : (crc >> 1);
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
    }

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
    }

    bool TCFArchive::ReadHeader(BinaryReader& reader) {
        if (mArchiveSize < kHeaderSize) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Archive is smaller than the TCF header. Size {}, expected at least {}",
                mArchiveSize,
                kHeaderSize
            );
            return false;
        }

        /*
            Read the entire fixed-size header into a buffer so we can
            CRC32 it before trusting any field inside it.

            Layout (see header):
                0x00  3   magic "TCF"
                0x03  1   version
                0x04  1   endianness
                0x05  3   reserved
                0x08  8   file_info_offset
                0x10  8   file_count
                0x18  8   data_offset
                0x20  4   header crc32
                0x24  92  reserved
        */
        std::array<uint8_t, kHeaderSize> header_bytes{};

        TCF_READ_OR_ERROR(
            reader.Read(header_bytes.data(), header_bytes.size()),
            "Failed to read TCF header bytes!"
        );

        if (std::string_view(
                reinterpret_cast<const char*>(header_bytes.data()),
                kTCFMagic.size()
            ) != kTCFMagic) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] TCF magic does not match, expected 'TCF' got '{}'",
                std::string_view(
                    reinterpret_cast<const char*>(header_bytes.data()),
                    kTCFMagic.size()
                )
            );
            return false;
        }

        mHeaderInfo.version = header_bytes[3];

        if (mHeaderInfo.version != kVersion) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] TCF archive version is not supported. Expected: {}, got {}",
                kVersion,
                mHeaderInfo.version
            );
            return false;
        }

        const uint8_t endianness = header_bytes[4];

        if (static_cast<Endianness>(endianness) != kEndianness) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Endianness set in TCF is not supported!"
            );
            return false;
        }

        for (size_t i = 5; i < 8; ++i) {
            if (header_bytes[i] != 0) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] Header reserved bytes must be zero!"
                );
                return false;
            }
        }

        mHeaderInfo.file_info_offset = ReadLEU64(header_bytes.data() + 0x08);
        mHeaderInfo.file_count       = ReadLEU64(header_bytes.data() + 0x10);
        mHeaderInfo.data_offset      = ReadLEU64(header_bytes.data() + 0x18);
        mHeaderInfo.crc              = ReadLEU32(header_bytes.data() + 0x20);

        for (size_t i = 0x24; i < kHeaderSize; ++i) {
            if (header_bytes[i] != 0) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] Header reserved bytes must be zero!"
                );
                return false;
            }
        }

        /*
            The header CRC is computed over the first kHeaderCrcLen bytes
            of the header (everything before the CRC field itself).
        */
        const uint32_t computed_crc =
            Crc32(header_bytes.data(), kHeaderCrcLen);

        if (computed_crc != mHeaderInfo.crc) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Header CRC32 mismatch. Expected {}, got {}",
                mHeaderInfo.crc,
                computed_crc
            );
            return false;
        }

        if (mHeaderInfo.data_offset < kHeaderSize) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Data offset {} is inside the header!",
                mHeaderInfo.data_offset
            );
            return false;
        }

        if (mHeaderInfo.data_offset > mArchiveSize) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Data offset {} is outside the archive. Archive size is {}",
                mHeaderInfo.data_offset,
                mArchiveSize
            );
            return false;
        }

        if (mHeaderInfo.file_info_offset < mHeaderInfo.data_offset) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] File info offset {} is before the data region (starts at {})!",
                mHeaderInfo.file_info_offset,
                mHeaderInfo.data_offset
            );
            return false;
        }

        if (mHeaderInfo.file_info_offset > mArchiveSize) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] File info offset {} is outside the archive. Archive size is {}",
                mHeaderInfo.file_info_offset,
                mArchiveSize
            );
            return false;
        }

        if (mHeaderInfo.file_count > kMaxFileCount) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Max file count exceeded. Max {}, got {}",
                kMaxFileCount,
                mHeaderInfo.file_count
            );
            return false;
        }

        if (mHeaderInfo.file_count > 0) {
            const uint64_t file_info_size =
                mArchiveSize - mHeaderInfo.file_info_offset;

            constexpr uint64_t kMinimumFileInfoSize =
                sizeof(uint64_t) +
                sizeof(uint64_t) +
                sizeof(uint64_t);

            if (file_info_size < kMinimumFileInfoSize ||
                mHeaderInfo.file_count > file_info_size / kMinimumFileInfoSize) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File count cannot fit inside the file info region!"
                );
                return false;
            }
        }

        return true;
    }

    bool TCFArchive::ReadFileInfo(BinaryReader& reader) {
        if (!reader.Seek(mHeaderInfo.file_info_offset, std::ios::beg)) {
            CE_LOG(
                LogLevel::Error,
                "[TCFArchive] Failed to seek to file info at offset {}!",
                mHeaderInfo.file_info_offset
            );
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

        for (uint64_t file_id = 0; file_id < mHeaderInfo.file_count; ++file_id) {
            FileInfo info;

            uint64_t path_size = 0;

            TCF_READ_OR_ERROR(
                reader.ReadU64(path_size),
                "Failed to read path size"
            );

            if (path_size == 0) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File {} has an empty path!",
                    file_id
                );
                return false;
            }

            if (path_size > kMaxPathSize) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File {} path size is too large. Maximum {}, got {}",
                    file_id,
                    kMaxPathSize,
                    path_size
                );
                return false;
            }

            info.file_path.resize(path_size);

            TCF_READ_OR_ERROR(
                reader.Read(info.file_path.data(), path_size),
                "Failed to read file path!"
            );

            // Validate the archive path.
            if (info.file_path.front() == '/' ||
                info.file_path.back() == '/') {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File {} has an invalid path '{}': leading/trailing '/'",
                    file_id,
                    info.file_path
                );
                return false;
            }

            size_t component_start = 0;

            while (component_start < info.file_path.size()) {
                const size_t separator =
                    info.file_path.find('/', component_start);

                const size_t component_end =
                    separator == std::string::npos
                        ? info.file_path.size()
                        : separator;

                const std::string_view component(
                    info.file_path.data() + component_start,
                    component_end - component_start
                );

                if (component.empty()) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] File {} has an empty path component: '{}'",
                        file_id,
                        info.file_path
                    );
                    return false;
                }

                if (component == "." || component == "..") {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] File {} contains forbidden path component in '{}'",
                        file_id,
                        info.file_path
                    );
                    return false;
                }

                for (char c : component) {
                    if (c == '\\' || c == '\0') {
                        CE_LOG(
                            LogLevel::Error,
                            "[TCFArchive] File {} contains an invalid character in path '{}'",
                            file_id,
                            info.file_path
                        );
                        return false;
                    }

                    // The TCF format specifies ASCII paths.
                    if (static_cast<unsigned char>(c) > 0x7F) {
                        CE_LOG(
                            LogLevel::Error,
                            "[TCFArchive] File {} contains a non-ASCII character in path '{}'",
                            file_id,
                            info.file_path
                        );
                        return false;
                    }
                }

                if (separator == std::string::npos) {
                    break;
                }

                component_start = separator + 1;
            }

            TCF_READ_OR_ERROR(
                reader.ReadU64(info.chunk_count),
                "Failed to read chunk count"
            );

            if (info.chunk_count > kMaxChunksPerFile) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File '{}' has too many chunks. Maximum {}, got {}",
                    info.file_path,
                    kMaxChunksPerFile,
                    info.chunk_count
                );
                return false;
            }

            TCF_READ_OR_ERROR(
                reader.ReadU64(info.chunk_block_offset),
                "Failed to read chunk block offset!"
            );

            if (info.chunk_count == 0) {
                if (info.chunk_block_offset != mHeaderInfo.file_info_offset) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] File '{}' has no chunks but its chunk block offset "
                        "does not point to the file info region!",
                        info.file_path
                    );
                    return false;
                }

                mFiles.push_back(std::move(info));
                continue;
            }

            if (info.chunk_block_offset < mHeaderInfo.data_offset) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File '{}' chunk block starts inside the header!",
                    info.file_path
                );
                return false;
            }

            if (info.chunk_block_offset >= mHeaderInfo.file_info_offset) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File '{}' chunk block starts inside or after the file info region!",
                    info.file_path
                );
                return false;
            }

            /*
                A chunk is laid out as:

                [uint64_chunk_id]
                [uint32_crc32]
                [uint8_compression_type]
                [uint64_compressed_size]
                [uint64_uncompressed_size]
                [uint64_data_end_offset]
                [data]
                [0x00]

                The fixed-size chunk header is therefore 37 bytes.
            */
            constexpr uint64_t kChunkHeaderSize =
                sizeof(uint64_t) +
                sizeof(uint32_t) +
                sizeof(uint8_t) +
                sizeof(uint64_t) +
                sizeof(uint64_t) +
                sizeof(uint64_t);

            if (info.chunk_count >
                (mHeaderInfo.file_info_offset - info.chunk_block_offset) /
                    (kChunkHeaderSize + 1)) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File '{}' has too many chunks to fit inside its chunk region!",
                    info.file_path
                );
                return false;
            }

            if (!reader.Seek(info.chunk_block_offset, std::ios::beg)) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] Failed to seek to chunk block for file '{}'!",
                    info.file_path
                );
                return false;
            }

            info.chunks.resize(info.chunk_count);

            uint64_t expected_offset = info.chunk_block_offset;

            for (uint64_t chunk_id = 0;
                 chunk_id < info.chunk_count;
                 ++chunk_id) {
                uint64_t stored_chunk_id = 0;

                TCF_READ_OR_ERROR(
                    reader.ReadU64(stored_chunk_id),
                    "Failed to read chunk ID!"
                );

                if (stored_chunk_id != chunk_id) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] File '{}' expected chunk ID {}, got {}",
                        info.file_path,
                        chunk_id,
                        stored_chunk_id
                    );
                    return false;
                }

                auto& chunk = info.chunks[chunk_id];

                TCF_READ_OR_ERROR(
                    reader.ReadU32(chunk.crc32),
                    "Failed to read chunk CRC32!"
                );

                uint8_t compression = 0;

                TCF_READ_OR_ERROR(
                    reader.ReadU8(compression),
                    "Failed to read chunk compression type!"
                );

                if (compression > static_cast<uint8_t>(CompressionType::None)) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Invalid compression type {} for file '{}'",
                        compression,
                        info.file_path
                    );
                    return false;
                }

                chunk.compression = static_cast<CompressionType>(compression);

                TCF_READ_OR_ERROR(
                    reader.ReadU64(chunk.compressed_size),
                    "Failed to read chunk compressed size!"
                );

                TCF_READ_OR_ERROR(
                    reader.ReadU64(chunk.uncompressed_size),
                    "Failed to read chunk uncompressed size!"
                );

                TCF_READ_OR_ERROR(
                    reader.ReadU64(chunk.offset),
                    "Failed to read chunk data end offset!"
                );

                /*
                    reader is now positioned immediately after the chunk
                    header, which is the beginning of the compressed data.
                */
                const uint64_t data_start =
                    expected_offset + kChunkHeaderSize;

                if (chunk.offset < data_start) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an end offset "
                        "before its data begins!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                if (chunk.offset >= mHeaderInfo.file_info_offset) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' extends into the file info region!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                const uint64_t physical_compressed_size =
                    chunk.offset - data_start;

                if (chunk.compressed_size != physical_compressed_size) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an invalid "
                        "compressed size. Metadata says {}, physical size is {}",
                        chunk_id,
                        info.file_path,
                        chunk.compressed_size,
                        physical_compressed_size
                    );
                    return false;
                }

                if (chunk.uncompressed_size > kFileChunkSize) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an uncompressed "
                        "size larger than the maximum chunk size. Maximum {}, got {}",
                        chunk_id,
                        info.file_path,
                        kFileChunkSize,
                        chunk.uncompressed_size
                    );
                    return false;
                }

                if (chunk.compression == CompressionType::None &&
                    chunk.compressed_size != chunk.uncompressed_size) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Uncompressed chunk {} in file '{}' has "
                        "different compressed and uncompressed sizes!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                /*
                    The byte at data_end_offset is the chunk terminator.
                */
                if (!reader.Seek(chunk.offset, std::ios::beg)) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Failed to seek to chunk terminator!"
                    );
                    return false;
                }

                uint8_t terminator = 0;

                TCF_READ_OR_ERROR(
                    reader.ReadU8(terminator),
                    "Failed to read chunk terminator!"
                );

                if (terminator != 0) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' is missing its "
                        "0x00 terminator!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                if (chunk.offset == UINT64_MAX) {
                    // Defensive check before adding one below.
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' has an invalid end offset!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                expected_offset = chunk.offset + 1;

                if (expected_offset > mHeaderInfo.file_info_offset) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] Chunk {} in file '{}' extends beyond "
                        "the chunk data region!",
                        chunk_id,
                        info.file_path
                    );
                    return false;
                }

                if (chunk_id + 1 < info.chunk_count) {
                    if (!reader.Seek(expected_offset, std::ios::beg)) {
                        CE_LOG(
                            LogLevel::Error,
                            "[TCFArchive] Failed to seek to next chunk!"
                        );
                        return false;
                    }
                }
            }

            chunk_blocks.push_back({
                info.chunk_block_offset,
                expected_offset
            });

            mFiles.push_back(std::move(info));
        }

        /*
            Make sure different files don't claim overlapping chunk blocks.
        */
        std::sort(
            chunk_blocks.begin(),
            chunk_blocks.end(),
            [](const ChunkBlockRange& a, const ChunkBlockRange& b) {
                return a.start < b.start;
            }
        );

        for (size_t i = 1; i < chunk_blocks.size(); ++i) {
            if (chunk_blocks[i].start < chunk_blocks[i - 1].end) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFArchive] File chunk blocks overlap!"
                );
                return false;
            }
        }

        return true;
    }

    bool TCFArchive::FileExists(const std::string& path) const {
        for (const auto& file : mFiles) {
            if (file.file_path == path) {
                return true;
            }
        }

        return false;
    }

    uint64_t TCFFile::Tell() {
        return mCurrentOffset;
    }

    bool TCFFile::Seek(int64_t offset, SeekMode mode) {
        uint64_t base_offset = 0;

        switch (mode) {
            case SeekMode::Start:
                base_offset = 0;
                break;

            case SeekMode::Current:
                base_offset = mCurrentOffset;
                break;

            case SeekMode::End:
                base_offset = mEndOffset;
                break;

            default:
                return false;
        }

        if (offset < 0) {
            const uint64_t distance =
                static_cast<uint64_t>(-(offset + 1)) + 1;

            if (distance > base_offset) {
                CE_LOG(
                    LogLevel::Error,
                    "[TCFFile] Offset goes before the start offset. "
                    "This will be truncated"
                );

                mCurrentOffset = 0;
                return false;
            }

            mCurrentOffset = base_offset - distance;
            return true;
        }

        const uint64_t distance = static_cast<uint64_t>(offset);

        if (distance > mEndOffset - base_offset) {
            CE_LOG(
                LogLevel::Error,
                "[TCFFile] Offset goes over the end offset. "
                "This will be truncated"
            );

            mCurrentOffset = mEndOffset;
            return false;
        }

        mCurrentOffset = base_offset + distance;
        return true;
    }

    bool TCFFile::IsValid() const {
        return mValid;
    }

    bool TCFFile::Eof() const {
        return mCurrentOffset >= mEndOffset;
    }

    TCFFile TCFArchive::OpenFile(const std::string& path) {
        for (size_t i = 0; i < mFiles.size(); ++i) {
            if (mFiles[i].file_path != path) {
                continue;
            }

            uint64_t file_size = 0;

            for (const FileInfo::Chunk& chunk : mFiles[i].chunks) {
                if (chunk.uncompressed_size >
                    std::numeric_limits<uint64_t>::max() - file_size) {
                    throw std::runtime_error(
                        "[TCFArchive] File size overflow"
                    );
                }

                file_size += chunk.uncompressed_size;
            }

            TCFFile file(*this, file_size);
            file.FileID = i;
            file.mCurrentOffset = 0;
            file.mValid = true;

            return file;
        }

        throw std::runtime_error(
            "[TCFArchive] File does not exist: " + path
        );
    }

    void TCFArchive::CloseFile(TCFFile& file) {
        if (&file.mTCFArchive != this) {
            return;
        }

        file.mValid = false;
        file.mCurrentOffset = file.mEndOffset;
    }


    bool TCFFile::ReadChunk(
        const TCFArchive::FileInfo::Chunk& chunk,
        std::vector<uint8_t>& output
    ) {
        if (!mValid) {
            return false;
        }

        if (chunk.uncompressed_size >
            static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return false;
        }

        if (chunk.compressed_size >
            static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return false;
        }

        if (chunk.offset < chunk.compressed_size) {
            return false;
        }

        const uint64_t data_start =
            chunk.offset - chunk.compressed_size;

        std::vector<uint8_t> compressed(
            static_cast<size_t>(chunk.compressed_size)
        );

        std::fstream& archive_stream = mTCFArchive.mFile;

        archive_stream.clear();
        archive_stream.seekg(
            static_cast<std::streamoff>(data_start),
            std::ios::beg
        );

        if (!archive_stream) {
            return false;
        }

        if (!compressed.empty()) {
            archive_stream.read(
                reinterpret_cast<char*>(compressed.data()),
                static_cast<std::streamsize>(compressed.size())
            );

            if (!archive_stream ||
                static_cast<size_t>(archive_stream.gcount()) != compressed.size()) {
                return false;
            }
        }

        output.resize(static_cast<size_t>(chunk.uncompressed_size));

        switch (chunk.compression) {
            case TCFArchive::CompressionType::None: {
                if (chunk.compressed_size != chunk.uncompressed_size) {
                    return false;
                }

                if (!compressed.empty()) {
                    std::memcpy(
                        output.data(),
                        compressed.data(),
                        compressed.size()
                    );
                }

                break;
            }

            case TCFArchive::CompressionType::Zstd: {
                const size_t result = ZSTD_decompress(
                    output.data(),
                    output.size(),
                    compressed.data(),
                    compressed.size()
                );

                if (ZSTD_isError(result) ||
                    result != output.size()) {
                    return false;
                }

                break;
            }

            case TCFArchive::CompressionType::LZ4: {
                if (compressed.size() >
                        static_cast<size_t>(std::numeric_limits<int>::max()) ||
                    output.size() >
                        static_cast<size_t>(std::numeric_limits<int>::max())) {
                    return false;
                }

                const int result = LZ4_decompress_safe(
                    reinterpret_cast<const char*>(compressed.data()),
                    reinterpret_cast<char*>(output.data()),
                    static_cast<int>(compressed.size()),
                    static_cast<int>(output.size())
                );

                if (result < 0 ||
                    static_cast<size_t>(result) != output.size()) {
                    return false;
                }

                break;
            }

            default:
                return false;
        }

        const uint32_t actual_crc32 = Crc32(output.data(), output.size());

        if (actual_crc32 != chunk.crc32) {
            CE_LOG(
                LogLevel::Error,
                "[TCFFile] CRC32 mismatch for chunk: expected {}, got {}",
                chunk.crc32,
                actual_crc32
            );
            return false;
        }

        return true;
    }

    TCFFile::~TCFFile() {
        mValid = false;
    }

    bool TCFFile::Read(void* destination, size_t size) {
        if (!mValid) {
            return false;
        }

        if (size == 0) {
            return true;
        }

        if (destination == nullptr) {
            return false;
        }

        if (mCurrentOffset > mEndOffset ||
            static_cast<uint64_t>(size) > mEndOffset - mCurrentOffset) {
            return false;
        }

        const auto& chunks = mTCFArchive.mFiles[FileID].chunks;

        auto* destination_bytes =
            static_cast<uint8_t*>(destination);

        size_t remaining = size;
        uint64_t chunk_start = 0;

        for (const auto& chunk : chunks) {
            const uint64_t chunk_end =
                chunk_start + chunk.uncompressed_size;

            if (mCurrentOffset >= chunk_end) {
                chunk_start = chunk_end;
                continue;
            }

            const uint64_t offset_in_chunk =
                mCurrentOffset - chunk_start;

            const uint64_t available_in_chunk =
                chunk.uncompressed_size - offset_in_chunk;

            const size_t bytes_to_copy =
                static_cast<size_t>(
                    std::min<uint64_t>(
                        available_in_chunk,
                        remaining
                    )
                );

            std::vector<uint8_t> uncompressed;

            if (!ReadChunk(chunk, uncompressed)) {
                return false;
            }

            if (offset_in_chunk + bytes_to_copy > uncompressed.size()) {
                return false;
            }

            std::memcpy(
                destination_bytes,
                uncompressed.data() + offset_in_chunk,
                bytes_to_copy
            );

            destination_bytes += bytes_to_copy;
            remaining -= bytes_to_copy;
            mCurrentOffset += bytes_to_copy;

            if (remaining == 0) {
                return true;
            }

            chunk_start = chunk_end;
        }

        return false;
    }

    bool TCFArchive::GetFileSize(const std::string& path, uint64_t& size) const {
        for (const auto& file : mFiles) {
            if (file.file_path != path) {
                continue;
            }

            size = 0;

            for (const auto& chunk : file.chunks) {
                if (chunk.uncompressed_size >
                    std::numeric_limits<uint64_t>::max() - size) {
                    CE_LOG(
                        LogLevel::Error,
                        "[TCFArchive] File '{}' size overflow!",
                        path
                    );
                    return false;
                }

                size += chunk.uncompressed_size;
            }

            return true;
        }

        return false;
    }
}