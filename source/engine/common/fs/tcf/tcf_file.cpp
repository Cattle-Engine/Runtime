#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

#include "engine/common/fs/tcf/tcf.hpp"
#include "engine/common/tracelog.hpp"

#include <lz4.h>
#include <zstd.h>

namespace CE::Common::FS::TCF {
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

    uint64_t TCFFile::Tell() const {
        return mCurrentOffset;
    }

    uint64_t TCFFile::Size() const {
        return mEndOffset;
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
            const uint64_t distance = static_cast<uint64_t>(-(offset + 1)) + 1;

            if (distance > base_offset) {
                CE_LOG(LogLevel::Error, "[TCFFile] Offset goes before the start offset. "
                                        "This will be truncated");

                mCurrentOffset = 0;
                return false;
            }

            mCurrentOffset = base_offset - distance;
            return true;
        }

        const uint64_t distance = static_cast<uint64_t>(offset);

        if (distance > mEndOffset - base_offset) {
            CE_LOG(LogLevel::Error, "[TCFFile] Offset goes over the end offset. "
                                    "This will be truncated");

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
        FileId file_id = 0;

        if (!FindFile(path, file_id)) {
            throw std::runtime_error("[TCFArchive] File does not exist: " + path);
        }

        const FileInfo& file_info = mFiles[file_id];

        uint64_t file_size = 0;

        for (const FileInfo::Chunk& chunk : file_info.chunks) {
            if (chunk.uncompressed_size > std::numeric_limits<uint64_t>::max() - file_size) {
                throw std::runtime_error("[TCFArchive] File size overflow");
            }

            file_size += chunk.uncompressed_size;
        }

        TCFFile file(*this, file_size);
        file.FileID = file_id;
        file.mCurrentOffset = 0;
        file.mValid = true;

        return file;
    }

    void TCFArchive::CloseFile(TCFFile& file) {
        if (&file.mTCFArchive != this) {
            return;
        }

        file.mValid = false;
        file.mCurrentOffset = file.mEndOffset;
    }

    bool TCFFile::ReadChunk(const TCFArchive::FileInfo::Chunk& chunk, std::vector<uint8_t>& output) {
        if (!mValid) {
            return false;
        }

        if (chunk.uncompressed_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return false;
        }

        if (chunk.compressed_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return false;
        }

        if (chunk.offset < chunk.compressed_size) {
            return false;
        }

        const uint64_t data_start = chunk.offset - chunk.compressed_size;

        std::vector<uint8_t> compressed(static_cast<size_t>(chunk.compressed_size));

        std::fstream& archive_stream = mTCFArchive.mFile;

        archive_stream.clear();
        archive_stream.seekg(static_cast<std::streamoff>(data_start), std::ios::beg);

        if (!archive_stream) {
            return false;
        }

        if (!compressed.empty()) {
            archive_stream.read(reinterpret_cast<char*>(compressed.data()),
                                static_cast<std::streamsize>(compressed.size()));

            if (!archive_stream || static_cast<size_t>(archive_stream.gcount()) != compressed.size()) {
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
                std::memcpy(output.data(), compressed.data(), compressed.size());
            }

            break;
        }

        case TCFArchive::CompressionType::Zstd: {
            const size_t result = ZSTD_decompress(output.data(), output.size(), compressed.data(), compressed.size());

            if (ZSTD_isError(result) || result != output.size()) {
                return false;
            }

            break;
        }

        case TCFArchive::CompressionType::LZ4: {
            if (compressed.size() > static_cast<size_t>(std::numeric_limits<int>::max()) ||
                output.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
                return false;
            }

            const int result = LZ4_decompress_safe(
                reinterpret_cast<const char*>(compressed.data()), reinterpret_cast<char*>(output.data()),
                static_cast<int>(compressed.size()), static_cast<int>(output.size()));

            if (result < 0 || static_cast<size_t>(result) != output.size()) {
                return false;
            }

            break;
        }

        default:
            return false;
        }

        const uint32_t actual_crc32 = Crc32(output.data(), output.size());

        if (actual_crc32 != chunk.crc32) {
            CE_LOG(LogLevel::Error, "[TCFFile] CRC32 mismatch for chunk: expected {}, got {}", chunk.crc32,
                   actual_crc32);
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

        if (mCurrentOffset > mEndOffset || static_cast<uint64_t>(size) > mEndOffset - mCurrentOffset) {
            return false;
        }

        const auto& chunks = mTCFArchive.mFiles[FileID].chunks;

        auto* destination_bytes = static_cast<uint8_t*>(destination);

        size_t remaining = size;
        uint64_t chunk_start = 0;

        for (size_t chunk_id = 0; chunk_id < chunks.size(); ++chunk_id) {
            const auto& chunk = chunks[chunk_id];

            const uint64_t chunk_end = chunk_start + chunk.uncompressed_size;

            if (mCurrentOffset >= chunk_end) {
                chunk_start = chunk_end;
                continue;
            }

            const uint64_t offset_in_chunk = mCurrentOffset - chunk_start;

            const uint64_t available_in_chunk = chunk.uncompressed_size - offset_in_chunk;

            const size_t bytes_to_copy = static_cast<size_t>(std::min<uint64_t>(available_in_chunk, remaining));

            std::vector<uint8_t>* uncompressed = nullptr;

            if (!GetChunkData(chunk_id, chunk, uncompressed)) {
                return false;
            }

            if (offset_in_chunk + bytes_to_copy > uncompressed->size()) {
                return false;
            }

            std::memcpy(destination_bytes, uncompressed->data() + offset_in_chunk, bytes_to_copy);

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

    bool TCFFile::GetChunkData(ChunkId chunk_id, const TCFArchive::FileInfo::Chunk& chunk,
                               std::vector<uint8_t>*& data) {
        ++mCacheClock;

        for (CachedChunk& cached : mChunkCache) {
            if (!cached.valid || cached.id != chunk_id) {
                continue;
            }

            cached.last_used = mCacheClock;
            data = &cached.data;
            return true;
        }

        size_t cache_index = 0;

        for (size_t i = 0; i < mChunkCache.size(); ++i) {
            if (!mChunkCache[i].valid) {
                cache_index = i;
                break;
            }

            if (mChunkCache[i].last_used < mChunkCache[cache_index].last_used) {
                cache_index = i;
            }
        }

        CachedChunk& cached = mChunkCache[cache_index];

        cached.data.clear();

        if (!ReadChunk(chunk, cached.data)) {
            cached.valid = false;
            return false;
        }

        cached.id = chunk_id;
        cached.last_used = mCacheClock;
        cached.valid = true;

        data = &cached.data;
        return true;
    }
} // namespace CE::Common::FS::TCF