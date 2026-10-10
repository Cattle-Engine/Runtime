#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include "ce_common/tcf.hpp"
#include "ce_common/detail/tracelog.hpp"

#include <lz4.h>
#include <lz4hc.h>
#include <zstd.h>

namespace CECommon::TCF {
    uint32_t Crc32(const uint8_t* data, size_t size);
    namespace {
        // Chunk header: id, crc32, compression, compressed size, uncompressed size, end offset
        constexpr uint64_t kChunkHeaderSize = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint64_t) +
                                              sizeof(uint64_t) + sizeof(uint64_t);

        constexpr uint64_t kDirectoryContentSize = sizeof(uint8_t) + sizeof(uint64_t);

        // Directory record without the name or contents: id, date, name size, parent, content end
        constexpr uint64_t kDirectoryHeaderSize =
            sizeof(uint64_t) + sizeof(int64_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);

        void StoreLE(uint8_t* destination, uint64_t value, size_t byte_count) {
            for (size_t i = 0; i < byte_count; ++i) {
                destination[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFFu);
            }
        }

        void PutLE(std::ostream& stream, uint64_t value, size_t byte_count) {
            std::array<uint8_t, 8> bytes{};

            StoreLE(bytes.data(), value, byte_count);

            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(byte_count));
        }

        void PutU8(std::ostream& stream, uint8_t value) {
            PutLE(stream, value, 1);
        }

        void PutU32(std::ostream& stream, uint32_t value) {
            PutLE(stream, value, 4);
        }

        void PutU64(std::ostream& stream, uint64_t value) {
            PutLE(stream, value, 8);
        }

        // Same rules as the reader's ValidateName / ValidatePath
        bool ValidateComponent(std::string_view name, uint64_t max_size) {
            if (name.empty() || name.size() > max_size) {
                CE_LOG(LogLevel::Error, "[TCFWriter] Path component has an invalid length: '{}'", name);
                return false;
            }

            if (name == "." || name == "..") {
                CE_LOG(LogLevel::Error, "[TCFWriter] Forbidden path component '{}'", name);
                return false;
            }

            for (char c : name) {
                if (c == '/' || c == '\\' || c == '\0' || static_cast<unsigned char>(c) > 0x7F) {
                    CE_LOG(LogLevel::Error, "[TCFWriter] Path component '{}' contains an invalid character", name);
                    return false;
                }
            }

            return true;
        }

        bool SplitArchivePath(const std::string& path, std::vector<std::string>& components, uint64_t max_name_size) {
            components.clear();

            std::string_view view(path);

            if (!view.empty() && view.front() == '/') {
                view.remove_prefix(1);
            }

            if (view.empty()) {
                CE_LOG(LogLevel::Error, "[TCFWriter] Archive path is empty");
                return false;
            }

            size_t component_start = 0;

            while (true) {
                const size_t separator = view.find('/', component_start);

                const size_t component_end = separator == std::string_view::npos ? view.size() : separator;

                const std::string_view component = view.substr(component_start, component_end - component_start);

                if (!ValidateComponent(component, max_name_size)) {
                    return false;
                }

                components.emplace_back(component);

                if (separator == std::string_view::npos) {
                    break;
                }

                component_start = separator + 1;
            }

            return true;
        }

        // Milliseconds since the unix epoch
        int64_t GetModifiedTimestamp(const std::filesystem::path& path) {
            std::error_code ec;

            const auto file_time = std::filesystem::last_write_time(path, ec);

            if (ec) {
                return 0;
            }

            const auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

            return std::chrono::duration_cast<std::chrono::milliseconds>(system_time.time_since_epoch()).count();
        }
    } // namespace

    TCFWriter::TCFWriter(const std::string& path) {
        static_assert(TCFArchive::kHeaderSize == 0x80, "Writer assumes a 128 byte header");

        mFile.open(path, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!mFile) {
            throw std::runtime_error("[TCFWriter] Failed to open file for writing: " + path);
        }

        // Placeholder header, patched in Finish(). File data starts right after it.
        const std::array<char, TCFArchive::kHeaderSize> placeholder{};

        mFile.write(placeholder.data(), static_cast<std::streamsize>(placeholder.size()));

        if (!mFile) {
            throw std::runtime_error("[TCFWriter] Failed to write header: " + path);
        }

        // Directory 0 is always the root: empty name, its own parent
        mDirectories.emplace_back();
    }

    TCFWriter::~TCFWriter() {
        if (!mFinished && mFile.is_open()) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Destroyed without calling Finish(), archive is incomplete");
        }
    }

    void TCFWriter::SetCompression(TCFArchive::CompressionType compression, int level) {
        mCompression = compression;
        mCompressionLevel = level;
    }

    void TCFWriter::SetChunkSize(size_t chunk_size) {
        if (chunk_size == 0 || chunk_size > static_cast<size_t>(TCFArchive::kFileChunkSize)) {
            throw std::runtime_error("[TCFWriter] Invalid chunk size");
        }

        mChunkSize = chunk_size;
    }

    bool TCFWriter::IsFinished() const {
        return mFinished;
    }

    bool TCFWriter::PrepareFile(const std::string& archive_path, std::vector<std::string>& components) const {
        if (mFinished || !mFile) {
            return false;
        }

        if (!SplitArchivePath(archive_path, components, TCFArchive::kMaxNameSize)) {
            return false;
        }

        if (mFiles.size() >= TCFArchive::kMaxFileCount) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Max file count reached");
            return false;
        }

        uint64_t current_directory = 0;

        for (size_t i = 0; i < components.size(); ++i) {
            const DirectoryEntry& directory = mDirectories[current_directory];

            const auto found = directory.children.find(components[i]);

            if (found == directory.children.end()) {
                // Everything below this point is new, make sure the directories fit
                const uint64_t new_directories = components.size() - 1 - i;

                if (new_directories > TCFArchive::kMaxFileCount - std::min<uint64_t>(mDirectories.size(),
                                                                                    TCFArchive::kMaxFileCount)) {
                    CE_LOG(LogLevel::Error, "[TCFWriter] Max directory count reached");
                    return false;
                }

                return true;
            }

            if (i + 1 == components.size()) {
                CE_LOG(LogLevel::Error, "[TCFWriter] Path already exists: {}", archive_path);
                return false;
            }

            if (found->second.type != TCFArchive::DirectoryContentType::Directory) {
                CE_LOG(LogLevel::Error, "[TCFWriter] '{}' is a file, but is used as a directory in: {}", components[i],
                       archive_path);
                return false;
            }

            current_directory = found->second.id;
        }

        return true;
    }

    bool TCFWriter::EnsureDirectory(const std::vector<std::string>& components, size_t count,
                                    uint64_t& directory_id) {
        directory_id = 0;

        for (size_t i = 0; i < count; ++i) {
            DirectoryEntry& current = mDirectories[directory_id];

            const auto found = current.children.find(components[i]);

            if (found != current.children.end()) {
                if (found->second.type != TCFArchive::DirectoryContentType::Directory) {
                    CE_LOG(LogLevel::Error, "[TCFWriter] '{}' already exists as a file", components[i]);
                    return false;
                }

                directory_id = found->second.id;
                continue;
            }

            if (mDirectories.size() >= TCFArchive::kMaxFileCount) {
                CE_LOG(LogLevel::Error, "[TCFWriter] Max directory count reached");
                return false;
            }

            const uint64_t new_id = mDirectories.size();

            DirectoryEntry child;
            child.name = components[i];
            child.parent = directory_id;

            const ChildRef reference{TCFArchive::DirectoryContentType::Directory, new_id};

            current.children.emplace(components[i], reference);
            current.contents.push_back(reference);

            // `current` is invalid after this
            mDirectories.push_back(std::move(child));

            directory_id = new_id;
        }

        return true;
    }

    void TCFWriter::Touch(uint64_t directory_id, int64_t date_modified) {
        uint64_t id = directory_id;

        while (true) {
            DirectoryEntry& directory = mDirectories[id];

            directory.date_modified = std::max(directory.date_modified, date_modified);

            if (id == 0) {
                break;
            }

            id = directory.parent;
        }
    }

    bool TCFWriter::CommitFile(const std::vector<std::string>& components, FileEntry&& entry) {
        uint64_t parent = 0;

        if (!EnsureDirectory(components, components.size() - 1, parent)) {
            return false;
        }

        const uint64_t file_id = mFiles.size();

        entry.name = components.back();
        entry.parent = parent;

        const ChildRef reference{TCFArchive::DirectoryContentType::File, file_id};

        mDirectories[parent].children.emplace(entry.name, reference);
        mDirectories[parent].contents.push_back(reference);

        Touch(parent, entry.date_modified);

        mFiles.push_back(std::move(entry));
        return true;
    }

    bool TCFWriter::AddDirectory(const std::string& archive_path, int64_t date_modified) {
        if (mFinished || !mFile) {
            return false;
        }

        std::vector<std::string> components;

        if (!SplitArchivePath(archive_path, components, TCFArchive::kMaxNameSize)) {
            return false;
        }

        uint64_t directory_id = 0;

        if (!EnsureDirectory(components, components.size(), directory_id)) {
            return false;
        }

        Touch(directory_id, date_modified);
        return true;
    }

    bool TCFWriter::AddFile(const std::string& archive_path, const void* data, size_t size, int64_t date_modified) {
        if (size != 0 && data == nullptr) {
            return false;
        }

        std::vector<std::string> components;

        if (!PrepareFile(archive_path, components)) {
            return false;
        }

        const uint64_t chunk_count = size / mChunkSize + (size % mChunkSize != 0 ? 1 : 0);

        if (chunk_count > TCFArchive::kMaxChunksPerFile) {
            CE_LOG(LogLevel::Error, "[TCFWriter] File '{}' needs too many chunks. Maximum {}, got {}", archive_path,
                   TCFArchive::kMaxChunksPerFile, chunk_count);
            return false;
        }

        FileEntry entry;
        entry.date_modified = date_modified;

        const auto* bytes = static_cast<const uint8_t*>(data);

        size_t written = 0;

        while (written < size) {
            const size_t chunk_size = std::min(mChunkSize, size - written);

            if (!WriteChunk(bytes + written, chunk_size, entry)) {
                return false;
            }

            written += chunk_size;
        }

        return CommitFile(components, std::move(entry));
    }

    bool TCFWriter::AddFileFromDisk(const std::string& archive_path, const std::filesystem::path& disk_path) {
        std::error_code ec;

        const uint64_t file_size = std::filesystem::file_size(disk_path, ec);

        if (ec) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to get size of file: {}", disk_path.string());
            return false;
        }

        std::vector<std::string> components;

        if (!PrepareFile(archive_path, components)) {
            return false;
        }

        const uint64_t chunk_count = file_size / mChunkSize + (file_size % mChunkSize != 0 ? 1 : 0);

        if (chunk_count > TCFArchive::kMaxChunksPerFile) {
            CE_LOG(LogLevel::Error, "[TCFWriter] File '{}' needs too many chunks. Maximum {}, got {}", archive_path,
                   TCFArchive::kMaxChunksPerFile, chunk_count);
            return false;
        }

        std::ifstream input(disk_path, std::ios::binary);

        if (!input) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to open file: {}", disk_path.string());
            return false;
        }

        FileEntry entry;
        entry.date_modified = GetModifiedTimestamp(disk_path);

        std::vector<uint8_t> buffer(mChunkSize);

        while (true) {
            input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

            const size_t bytes_read = static_cast<size_t>(input.gcount());

            if (bytes_read == 0) {
                break;
            }

            if (!WriteChunk(buffer.data(), bytes_read, entry)) {
                return false;
            }
        }

        if (input.bad()) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to read file: {}", disk_path.string());
            return false;
        }

        return CommitFile(components, std::move(entry));
    }

    bool TCFWriter::AddDirectoryFromDisk(const std::string& archive_path, const std::filesystem::path& disk_path) {
        std::error_code ec;

        if (!std::filesystem::is_directory(disk_path, ec)) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Not a directory: {}", disk_path.string());
            return false;
        }

        std::vector<std::filesystem::path> entries;

        const std::filesystem::recursive_directory_iterator end;

        for (std::filesystem::recursive_directory_iterator it(disk_path, ec); !ec && it != end; it.increment(ec)) {
            entries.push_back(it->path());
        }

        if (ec) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to iterate directory: {}", disk_path.string());
            return false;
        }

        // Sorted so the output is deterministic and parents come before their children
        std::sort(entries.begin(), entries.end());

        std::string prefix = archive_path;

        while (!prefix.empty() && prefix.back() == '/') {
            prefix.pop_back();
        }

        if (!prefix.empty() && !AddDirectory(prefix, GetModifiedTimestamp(disk_path))) {
            return false;
        }

        for (const std::filesystem::path& entry : entries) {
            const std::string relative = entry.lexically_relative(disk_path).generic_string();
            const std::string target = prefix.empty() ? relative : prefix + '/' + relative;

            if (std::filesystem::is_symlink(entry, ec)) {
                continue;
            }

            if (std::filesystem::is_directory(entry, ec)) {
                if (!AddDirectory(target, GetModifiedTimestamp(entry))) {
                    return false;
                }
            } else if (std::filesystem::is_regular_file(entry, ec)) {
                if (!AddFileFromDisk(target, entry)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool TCFWriter::Compress(const uint8_t* data, size_t size, std::vector<uint8_t>& output) const {
        switch (mCompression) {
        case TCFArchive::CompressionType::Zstd: {
            output.resize(ZSTD_compressBound(size));

            const size_t result = ZSTD_compress(output.data(), output.size(), data, size, mCompressionLevel);

            if (ZSTD_isError(result)) {
                return false;
            }

            output.resize(result);
            return true;
        }

        case TCFArchive::CompressionType::LZ4: {
            if (size > static_cast<size_t>(LZ4_MAX_INPUT_SIZE)) {
                return false;
            }

            const int bound = LZ4_compressBound(static_cast<int>(size));

            if (bound <= 0) {
                return false;
            }

            output.resize(static_cast<size_t>(bound));

            const int result = (mCompressionLevel > 0)
                                   ? LZ4_compress_HC(reinterpret_cast<const char*>(data),
                                                     reinterpret_cast<char*>(output.data()), static_cast<int>(size),
                                                     bound, mCompressionLevel)
                                   : LZ4_compress_default(reinterpret_cast<const char*>(data),
                                                          reinterpret_cast<char*>(output.data()),
                                                          static_cast<int>(size), bound);

            if (result <= 0) {
                return false;
            }

            output.resize(static_cast<size_t>(result));
            return true;
        }

        default:
            return false;
        }
    }

    bool TCFWriter::WriteChunk(const uint8_t* data, size_t size, FileEntry& entry) {
        if (entry.chunk_count >= TCFArchive::kMaxChunksPerFile) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Too many chunks in file");
            return false;
        }

        const uint32_t crc32 = Crc32(data, size);

        const uint8_t* payload = data;
        size_t payload_size = size;
        TCFArchive::CompressionType compression = TCFArchive::CompressionType::None;

        // Only keep the compressed version if it is actually smaller
        if (mCompression != TCFArchive::CompressionType::None && Compress(data, size, mScratch) &&
            mScratch.size() < size) {
            payload = mScratch.data();
            payload_size = mScratch.size();
            compression = mCompression;
        }

        const uint64_t chunk_start = static_cast<uint64_t>(mFile.tellp());

        // The chunk header stores the offset of the terminator byte, which follows the data
        const uint64_t end_offset = chunk_start + kChunkHeaderSize + payload_size;

        PutU64(mFile, entry.chunk_count);
        PutU32(mFile, crc32);
        PutU8(mFile, static_cast<uint8_t>(compression));
        PutU64(mFile, payload_size);
        PutU64(mFile, size);
        PutU64(mFile, end_offset);

        mFile.write(reinterpret_cast<const char*>(payload), static_cast<std::streamsize>(payload_size));

        PutU8(mFile, 0);

        if (!mFile) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to write chunk");
            return false;
        }

        if (entry.chunk_count == 0) {
            entry.chunk_block_offset = chunk_start;
        }

        ++entry.chunk_count;
        return true;
    }

    bool TCFWriter::WriteFileInfo() {
        for (uint64_t file_id = 0; file_id < mFiles.size(); ++file_id) {
            const FileEntry& file = mFiles[file_id];

            PutU64(mFile, file_id);
            PutU64(mFile, static_cast<uint64_t>(file.date_modified));
            PutU32(mFile, static_cast<uint32_t>(file.name.size()));

            mFile.write(file.name.data(), static_cast<std::streamsize>(file.name.size()));

            PutU64(mFile, file.parent);
            PutU64(mFile, file.chunk_count);
            PutU64(mFile, file.chunk_block_offset);
        }

        return static_cast<bool>(mFile);
    }

    bool TCFWriter::WriteDirectoryTable(uint64_t directory_table_offset) {
        uint64_t position = directory_table_offset;

        for (uint64_t directory_id = 0; directory_id < mDirectories.size(); ++directory_id) {
            const DirectoryEntry& directory = mDirectories[directory_id];

            const uint64_t content_end =
                position + kDirectoryHeaderSize + directory.name.size() + directory.contents.size() * kDirectoryContentSize;

            PutU64(mFile, directory_id);
            PutU64(mFile, static_cast<uint64_t>(directory.date_modified));
            PutU32(mFile, static_cast<uint32_t>(directory.name.size()));

            mFile.write(directory.name.data(), static_cast<std::streamsize>(directory.name.size()));

            PutU64(mFile, directory.parent);
            PutU64(mFile, content_end);

            for (const ChildRef& content : directory.contents) {
                PutU8(mFile, static_cast<uint8_t>(content.type));
                PutU64(mFile, content.id);
            }

            position = content_end;
        }

        return static_cast<bool>(mFile);
    }

    bool TCFWriter::WriteHeader(uint64_t file_info_offset, uint64_t directory_table_offset) {
        std::array<uint8_t, TCFArchive::kHeaderSize> header{};

        std::memcpy(header.data(), TCFArchive::kTCFMagic.data(), TCFArchive::kTCFMagic.size());

        header[3] = static_cast<uint8_t>(TCFArchive::kVersion);
        header[4] = static_cast<uint8_t>(TCFArchive::kEndianness);

        StoreLE(header.data() + 0x08, file_info_offset, 8);
        StoreLE(header.data() + 0x10, mFiles.size(), 8);
        StoreLE(header.data() + 0x18, TCFArchive::kHeaderSize, 8);

        // The CRC covers everything before the CRC field
        StoreLE(header.data() + 0x20, Crc32(header.data(), TCFArchive::kHeaderCrcLen), 4);

        StoreLE(header.data() + 0x24, directory_table_offset, 8);
        StoreLE(header.data() + 0x2C, mDirectories.size(), 8);

        mFile.seekp(0, std::ios::beg);
        mFile.write(reinterpret_cast<const char*>(header.data()), static_cast<std::streamsize>(header.size()));

        return static_cast<bool>(mFile);
    }

    bool TCFWriter::Finish() {
        if (mFinished) {
            return true;
        }

        if (!mFile) {
            return false;
        }

        mFile.seekp(0, std::ios::end);

        const uint64_t file_info_offset = static_cast<uint64_t>(mFile.tellp());

        if (!WriteFileInfo()) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to write file info");
            return false;
        }

        const uint64_t directory_table_offset = static_cast<uint64_t>(mFile.tellp());

        if (!WriteDirectoryTable(directory_table_offset)) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to write directory table");
            return false;
        }

        if (!WriteHeader(file_info_offset, directory_table_offset)) {
            CE_LOG(LogLevel::Error, "[TCFWriter] Failed to write header");
            return false;
        }

        mFile.flush();
        mFile.close();

        if (mFile.fail()) {
            return false;
        }

        mFinished = true;
        return true;
    }
} // namespace CECommon::TCF