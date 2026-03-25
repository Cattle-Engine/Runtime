/* SPDX-License-Identifier: Zlib
 * Copyright (c) 2026 Tbaggerofsteam, Tbaggeroftheuk
 */

#include "engine/common/tcf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>

#define TCF_MAGIC      "TCF"
#define VERSION_V1     1
#define VERSION_V2     2
#define ENDIANNESS     0
#define SHIFT_BITS     2
#define BUFFER_SIZE    (64 * 1024)
constexpr uint64_t TCF_HEADER_SIZE_V1 = 18;
constexpr uint64_t TCF_HEADER_SIZE_V2 = 64;
constexpr uint64_t TCF_HEADER_CRC32_V1_LEN = 14;
constexpr uint64_t TCF_HEADER_CRC32_V2_LEN = 56;

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define PATH_SEP '\\'
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEP '/'
#endif

static uint32_t crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}


static uint8_t shift_table[256];
static uint8_t unshift_table[256];
static bool tables_initialized = false;

static void init_tables()
{
    if (tables_initialized)
        return;

    for (int i = 0; i < 256; i++) {
        shift_table[i] = (uint8_t)(((i << SHIFT_BITS) & 0xFF) |
                                   (i >> (8 - SHIFT_BITS)));
        unshift_table[i] = (uint8_t)((i >> SHIFT_BITS) | 
                                     (i << (8 - SHIFT_BITS)));
    }

    tables_initialized = true;
}

struct TCFEntry {
    std::string path;
    uint64_t offset;
    uint64_t size;
};

static std::vector<TCFEntry> entries;

static void add_entry(const std::string& path, uint64_t size)
{
    entries.emplace_back(TCFEntry{path, 0, size});
}

int ensure_dirs(const char *path) {
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());
    return 0;
}

static uint16_t read_u16(std::ifstream& f)
{
    uint8_t b[2];
    f.read(reinterpret_cast<char*>(b), 2);
    if (!f) return 0;
    return static_cast<uint16_t>(b[0] | (b[1] << 8));
}

static uint32_t read_u32(std::ifstream& f)
{
    uint8_t b[4];
    f.read(reinterpret_cast<char*>(b), 4);
    if (!f) return 0;
    return static_cast<uint32_t>(
        b[0] |
        (b[1] << 8) |
        (b[2] << 16) |
        (b[3] << 24)
    );
}

static uint64_t read_u64(std::ifstream& f)
{
    uint8_t b[8];
    f.read(reinterpret_cast<char*>(b), 8);
    if (!f) return 0;
    return static_cast<uint64_t>(b[0]) |
           (static_cast<uint64_t>(b[1]) << 8) |
           (static_cast<uint64_t>(b[2]) << 16) |
           (static_cast<uint64_t>(b[3]) << 24) |
           (static_cast<uint64_t>(b[4]) << 32) |
           (static_cast<uint64_t>(b[5]) << 40) |
           (static_cast<uint64_t>(b[6]) << 48) |
           (static_cast<uint64_t>(b[7]) << 56);
}

static void write_u16_le(std::ofstream& f, uint16_t v)
{
    uint8_t b[2] = {
        static_cast<uint8_t>(v & 0xFF),
        static_cast<uint8_t>((v >> 8) & 0xFF),
    };
    f.write(reinterpret_cast<const char*>(b), 2);
}

static void write_u64_le(std::ofstream& f, uint64_t v)
{
    uint8_t b[8] = {
        static_cast<uint8_t>(v & 0xFF),
        static_cast<uint8_t>((v >> 8) & 0xFF),
        static_cast<uint8_t>((v >> 16) & 0xFF),
        static_cast<uint8_t>((v >> 24) & 0xFF),
        static_cast<uint8_t>((v >> 32) & 0xFF),
        static_cast<uint8_t>((v >> 40) & 0xFF),
        static_cast<uint8_t>((v >> 48) & 0xFF),
        static_cast<uint8_t>((v >> 56) & 0xFF),
    };
    f.write(reinterpret_cast<const char*>(b), 8);
}

static uint32_t le_u32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t le_u64(const uint8_t* p)
{
    return static_cast<uint64_t>(p[0]) |
           (static_cast<uint64_t>(p[1]) << 8) |
           (static_cast<uint64_t>(p[2]) << 16) |
           (static_cast<uint64_t>(p[3]) << 24) |
           (static_cast<uint64_t>(p[4]) << 32) |
           (static_cast<uint64_t>(p[5]) << 40) |
           (static_cast<uint64_t>(p[6]) << 48) |
           (static_cast<uint64_t>(p[7]) << 56);
}

static std::string normalize_inner_path(const char *inner_path)
{
    if (!inner_path)
        return {};

    std::string s(inner_path);
    for (char &ch : s) {
        if (ch == '\\')
            ch = '/';
    }

    while (!s.empty() && s.front() == '/')
        s.erase(s.begin());

    return s;
}

static bool is_safe_inner_path(const std::string& p)
{
    if (p.empty())
        return false;
    if (p.find("..") != std::string::npos)
        return false;
    if (!p.empty() && (p[0] == '/' || p[0] == '\\'))
        return false;
    if (p.find(':') != std::string::npos)
        return false;
    return true;
}

struct TCFHeaderInfo {
    uint8_t version = 0;
    uint64_t index_offset = 0;
    uint64_t file_count = 0;
    uint64_t data_offset = 0;
};

static uint64_t get_file_size(std::ifstream& f)
{
    std::streampos cur = f.tellg();
    f.seekg(0, std::ios::end);
    std::streampos end = f.tellg();
    f.seekg(cur, std::ios::beg);
    if (end < 0)
        return 0;
    return static_cast<uint64_t>(end);
}

static int read_tcf_header(std::ifstream& f, TCFHeaderInfo& info)
{
    uint8_t prefix[4];
    f.read(reinterpret_cast<char*>(prefix), sizeof(prefix));
    if (!f || f.gcount() != static_cast<std::streamsize>(sizeof(prefix)))
        return TCF_ERR_FORMAT;

    if (std::memcmp(prefix, TCF_MAGIC, 3) != 0)
        return TCF_ERR_FORMAT;

    info.version = prefix[3];

    if (info.version == VERSION_V1) {
        uint8_t header[static_cast<size_t>(TCF_HEADER_SIZE_V1)];
        std::memcpy(header, prefix, sizeof(prefix));
        f.read(reinterpret_cast<char*>(header + sizeof(prefix)), static_cast<std::streamsize>(TCF_HEADER_SIZE_V1 - sizeof(prefix)));
        if (!f || f.gcount() != static_cast<std::streamsize>(TCF_HEADER_SIZE_V1 - sizeof(prefix)))
            return TCF_ERR_FORMAT;

        uint32_t expected_crc = le_u32(header + 14);
        if (crc32(header, static_cast<size_t>(TCF_HEADER_CRC32_V1_LEN)) != expected_crc)
            return TCF_ERR_CRC;

        info.index_offset = le_u32(header + 6);
        info.file_count = le_u32(header + 10);
        info.data_offset = TCF_HEADER_SIZE_V1;
    } else if (info.version == VERSION_V2) {
        uint8_t header[static_cast<size_t>(TCF_HEADER_SIZE_V2)];
        std::memcpy(header, prefix, sizeof(prefix));
        f.read(reinterpret_cast<char*>(header + sizeof(prefix)), static_cast<std::streamsize>(TCF_HEADER_SIZE_V2 - sizeof(prefix)));
        if (!f || f.gcount() != static_cast<std::streamsize>(TCF_HEADER_SIZE_V2 - sizeof(prefix)))
            return TCF_ERR_FORMAT;

        uint32_t expected_crc = le_u32(header + 56);
        if (crc32(header, static_cast<size_t>(TCF_HEADER_CRC32_V2_LEN)) != expected_crc)
            return TCF_ERR_CRC;

        info.index_offset = le_u64(header + 8);
        info.file_count = le_u64(header + 16);
        info.data_offset = le_u64(header + 24);

        if (info.data_offset < TCF_HEADER_SIZE_V2)
            return TCF_ERR_FORMAT;
    } else {
        return TCF_ERR_FORMAT;
    }

    if (info.index_offset < info.data_offset)
        return TCF_ERR_FORMAT;

    uint64_t file_size = get_file_size(f);
    if (file_size == 0)
        return TCF_ERR_IO;
    if (info.index_offset > file_size)
        return TCF_ERR_FORMAT;

    return TCF_OK;
}

static int read_tcf_index_entry(std::ifstream& f, uint8_t version, std::string& path_out, uint64_t& offset_out, uint64_t& size_out)
{
    uint16_t path_len = read_u16(f);
    if (!f)
        return TCF_ERR_IO;

    std::string path(path_len, '\0');
    if (path_len > 0) {
        f.read(&path[0], path_len);
        if (!f)
            return TCF_ERR_IO;
    }

    uint64_t offset = 0;
    uint64_t size = 0;
    if (version == VERSION_V1) {
        offset = read_u32(f);
        if (!f) return TCF_ERR_IO;
        size = read_u32(f);
        if (!f) return TCF_ERR_IO;
    } else {
        offset = read_u64(f);
        if (!f) return TCF_ERR_IO;
        size = read_u64(f);
        if (!f) return TCF_ERR_IO;
    }

    path_out = std::move(path);
    offset_out = offset;
    size_out = size;
    return TCF_OK;
}

static void walk_dir(const std::filesystem::path& base, const std::filesystem::path& rel = "")
{
    std::filesystem::path current = base / rel;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(current)) {
        if (entry.is_regular_file()) {
            std::filesystem::path relative_path = std::filesystem::relative(entry.path(), base);
            std::string path_str = relative_path.generic_string(); // Use forward slashes
            uint64_t size = static_cast<uint64_t>(entry.file_size());
            add_entry(path_str, size);
        }
    }
}

int tcf_pack(const char *input_dir, const char *out_path)
{
    init_tables();
    entries.clear();
    walk_dir(input_dir);

    std::sort(entries.begin(), entries.end(),
              [](const TCFEntry& a, const TCFEntry& b) { return a.size < b.size; });

    std::ofstream out(out_path, std::ios::binary);
    if (!out) return TCF_ERR_IO;

    uint8_t header[static_cast<size_t>(TCF_HEADER_SIZE_V2)] = {0};
    out.write(reinterpret_cast<char*>(header), static_cast<std::streamsize>(sizeof(header)));
    if (!out)
        return TCF_ERR_IO;

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uint64_t offset = 0;

    for (auto& entry : entries) {
        entry.offset = offset;

        std::filesystem::path full_path = std::filesystem::path(input_dir) / entry.path;
        std::ifstream in(full_path, std::ios::binary);
        if (!in) return TCF_ERR_IO;

        size_t r;
        while (in) {
            in.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
            r = static_cast<size_t>(in.gcount());
            if (r == 0)
                break;

            for (size_t j = 0; j < r; j++) {
                buffer[j] = shift_table[buffer[j]];
            }

            out.write(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(r));
        }

        offset += entry.size;
    }

    std::streampos index_pos = out.tellp();
    if (index_pos < 0)
        return TCF_ERR_IO;
    uint64_t index_offset = static_cast<uint64_t>(index_pos);

    for (const auto& entry : entries) {
        if (entry.path.size() > std::numeric_limits<uint16_t>::max())
            return TCF_ERR_ARG;

        uint16_t len = static_cast<uint16_t>(entry.path.size());
        write_u16_le(out, len);
        out.write(entry.path.data(), static_cast<std::streamsize>(len));
        write_u64_le(out, entry.offset);
        write_u64_le(out, entry.size);
        if (!out)
            return TCF_ERR_IO;
    }

    out.write("EOF", 3);
    if (!out)
        return TCF_ERR_IO;

    std::memcpy(header + 0, TCF_MAGIC, 3);
    header[3] = VERSION_V2;
    header[4] = 0;           // endianness marker (unused, little-endian)
    header[5] = 0;           // flags

    // 6-7 reserved

    // index_offset @ 8
    {
        uint64_t v = index_offset;
        for (int i = 0; i < 8; ++i) header[8 + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
    }

    // file_count @ 16
    {
        uint64_t v = static_cast<uint64_t>(entries.size());
        for (int i = 0; i < 8; ++i) header[16 + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
    }

    // data_offset/header_size @ 24
    {
        uint64_t v = TCF_HEADER_SIZE_V2;
        for (int i = 0; i < 8; ++i) header[24 + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
    }

    uint32_t crc = crc32(header, static_cast<size_t>(TCF_HEADER_CRC32_V2_LEN));
    header[56] = static_cast<uint8_t>(crc & 0xFF);
    header[57] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    header[58] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    header[59] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    out.seekp(0);
    out.write(reinterpret_cast<char*>(header), static_cast<std::streamsize>(sizeof(header)));
    if (!out)
        return TCF_ERR_IO;

    return TCF_OK;
}

int tcf_load_file(const char *tcf_path,
                  const char *inner_path,
                  uint8_t **out_data,
                  uint64_t *out_size)
{
    if (!tcf_path || !inner_path || !out_data || !out_size)
        return TCF_ERR_ARG;

    *out_data = nullptr;
    *out_size = 0;

    init_tables();

    std::ifstream f(tcf_path, std::ios::binary);
    if (!f)
        return TCF_ERR_IO;

    TCFHeaderInfo header;
    int header_res = read_tcf_header(f, header);
    if (header_res != TCF_OK)
        return header_res;

    std::string wanted = normalize_inner_path(inner_path);
    if (!is_safe_inner_path(wanted))
        return TCF_ERR_ARG;

    f.seekg(static_cast<std::streamoff>(header.index_offset), std::ios::beg);
    if (!f)
        return TCF_ERR_IO;

    bool found = false;
    uint64_t found_offset = 0;
    uint64_t found_size = 0;

    for (uint64_t i = 0; i < header.file_count; i++) {
        std::string path;
        uint64_t offset = 0;
        uint64_t size = 0;
        int entry_res = read_tcf_index_entry(f, header.version, path, offset, size);
        if (entry_res != TCF_OK)
            return entry_res;

        if (!is_safe_inner_path(path))
            continue;

        if (path == wanted) {
            found = true;
            found_offset = offset;
            found_size = size;
            break;
        }
    }

    if (!found)
        return TCF_ERR_NOT_FOUND;

    uint64_t payload_size = header.index_offset - header.data_offset;
    if (found_offset > payload_size || found_size > payload_size || found_offset + found_size > payload_size)
        return TCF_ERR_FORMAT;

    if (found_size == 0) {
        *out_data = nullptr;
        *out_size = 0;
        return TCF_OK;
    }

    if (found_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        return TCF_ERR_MEMORY;

    uint8_t *data = static_cast<uint8_t*>(std::malloc(static_cast<size_t>(found_size)));
    if (!data)
        return TCF_ERR_MEMORY;

    uint64_t abs_off = header.data_offset + found_offset;
    if (abs_off > header.index_offset) {
        std::free(data);
        return TCF_ERR_FORMAT;
    }

    f.seekg(static_cast<std::streamoff>(abs_off), std::ios::beg);
    if (!f) {
        std::free(data);
        return TCF_ERR_IO;
    }

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uint64_t remaining = found_size;
    size_t written = 0;

    while (remaining > 0) {
        size_t chunk = static_cast<size_t>(std::min<uint64_t>(remaining, static_cast<uint64_t>(BUFFER_SIZE)));
        f.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk));
        if (!f || static_cast<size_t>(f.gcount()) != chunk) {
            std::free(data);
            return TCF_ERR_IO;
        }

        for (size_t j = 0; j < chunk; j++) {
            data[written + j] = unshift_table[buffer[j]];
        }

        written += chunk;
        remaining -= chunk;
    }

    *out_data = data;
    *out_size = found_size;
    return TCF_OK;
}

void tcf_free(void *ptr)
{
    std::free(ptr);
}

int tcf_extract(const char *tcf_path, const char *output_dir)
{
    init_tables();

    std::ifstream f(tcf_path, std::ios::binary);
    if (!f)
        return TCF_ERR_IO;

    TCFHeaderInfo header;
    int header_res = read_tcf_header(f, header);
    if (header_res != TCF_OK)
        return header_res;

    if (header.file_count > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        return TCF_ERR_FORMAT;

    // Read index entries into memory first (so we can seek into payload safely).
    std::vector<TCFEntry> local_entries;
    local_entries.reserve(static_cast<size_t>(header.file_count));

    f.seekg(static_cast<std::streamoff>(header.index_offset), std::ios::beg);
    if (!f)
        return TCF_ERR_IO;

    for (uint64_t i = 0; i < header.file_count; ++i) {
        std::string path;
        uint64_t offset = 0;
        uint64_t size = 0;
        int entry_res = read_tcf_index_entry(f, header.version, path, offset, size);
        if (entry_res != TCF_OK)
            return entry_res;

        if (!is_safe_inner_path(path))
            continue;

        local_entries.push_back({std::move(path), offset, size});
    }

    // Optional EOF marker validation.
    char eof_marker[3];
    f.read(eof_marker, 3);
    if (f && std::memcmp(eof_marker, "EOF", 3) != 0)
        return TCF_ERR_FORMAT;

    const uint64_t payload_size = header.index_offset - header.data_offset;
    std::vector<uint8_t> buffer(BUFFER_SIZE);

    for (const auto& entry : local_entries) {
        if (entry.offset > payload_size || entry.size > payload_size || entry.offset + entry.size > payload_size)
            return TCF_ERR_FORMAT;

        std::filesystem::path full_path = std::filesystem::path(output_dir) / entry.path;
        ensure_dirs(full_path.string().c_str());

        std::ofstream out(full_path, std::ios::binary);
        if (!out)
            return TCF_ERR_IO;

        uint64_t abs_off = header.data_offset + entry.offset;
        f.seekg(static_cast<std::streamoff>(abs_off), std::ios::beg);
        if (!f)
            return TCF_ERR_IO;

        uint64_t remaining = entry.size;
        while (remaining > 0) {
            size_t chunk = static_cast<size_t>(std::min<uint64_t>(remaining, static_cast<uint64_t>(BUFFER_SIZE)));
            f.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk));
            if (!f || static_cast<size_t>(f.gcount()) != chunk)
                return TCF_ERR_IO;

            for (size_t j = 0; j < chunk; ++j)
                buffer[j] = unshift_table[buffer[j]];

            out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(chunk));
            if (!out)
                return TCF_ERR_IO;

            remaining -= chunk;
        }
    }

    return TCF_OK;
}


struct tcf_vfs_t {
    std::ifstream file;                // Open archive file
    uint8_t version;                  // TCF version
    uint64_t index_offset;             // Offset to index table in the file
    uint64_t file_count;               // Number of files in archive
    uint64_t data_offset;              // Offset to payload start
    std::vector<TCFEntry> entries;     // In-memory index (path, offset, size)
};

struct tcf_file_t {
    tcf_vfs_t *vfs;                    // Parent VFS
    uint64_t offset;                   // File data start (relative to payload start)
    uint64_t size;                     // Total size of the file
    uint64_t position;                 // Current read position (0..size)
};

/**
 * Opens a TCF archive for VFS access.
 * @param tcf_path Path to the .tcf archive file.
 * @param vfs_out Output pointer to the allocated VFS handle.
 * @return TCF_OK on success, error code otherwise.
 */

uint64_t tcf_vfs_file_size(tcf_file_t *file) {
    if (!file)
        return 0;
    return file->size;
}

int tcf_vfs_open(const char *tcf_path, tcf_vfs_t **vfs_out) {
    if (!tcf_path || !vfs_out)
        return TCF_ERR_ARG;

    init_tables();

    std::unique_ptr<tcf_vfs_t> vfs(new tcf_vfs_t());
    vfs->file.open(tcf_path, std::ios::binary);
    if (!vfs->file)
        return TCF_ERR_IO;

    TCFHeaderInfo header;
    int header_res = read_tcf_header(vfs->file, header);
    if (header_res != TCF_OK)
        return header_res;

    vfs->version = header.version;
    vfs->index_offset = header.index_offset;
    vfs->file_count = header.file_count;
    vfs->data_offset = header.data_offset;

    if (vfs->file_count > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        return TCF_ERR_FORMAT;

    // Seek to index table and read all entries
    vfs->file.seekg(static_cast<std::streamoff>(vfs->index_offset), std::ios::beg);
    if (!vfs->file)
        return TCF_ERR_IO;

    vfs->entries.clear();
    vfs->entries.reserve(static_cast<size_t>(vfs->file_count));

    for (uint64_t i = 0; i < vfs->file_count; ++i) {
        std::string path;
        uint64_t offset = 0;
        uint64_t size = 0;
        int entry_res = read_tcf_index_entry(vfs->file, vfs->version, path, offset, size);
        if (entry_res != TCF_OK)
            return entry_res;

        if (is_safe_inner_path(path))
            vfs->entries.push_back({std::move(path), offset, size});
    }

    // Optional EOF marker validation.
    char eof_marker[3];
    vfs->file.read(eof_marker, 3);
    if (vfs->file && std::memcmp(eof_marker, "EOF", 3) != 0)
        return TCF_ERR_FORMAT;

    *vfs_out = vfs.release();
    return TCF_OK;
}

/**
 * Closes a VFS handle and releases all associated resources.
 * @param vfs The VFS handle to close (can be NULL).
 */
void tcf_vfs_close(tcf_vfs_t *vfs) {
    if (vfs) {
        vfs->file.close();
        delete vfs;
    }
}

/**
 * Opens a file inside the archive for reading.
 * @param vfs The VFS handle.
 * @param inner_path The virtual path of the file inside the archive.
 * @param file_out Output pointer to the file handle.
 * @return TCF_OK on success, error code otherwise.
 */
int tcf_vfs_open_file(tcf_vfs_t *vfs, const char *inner_path, tcf_file_t **file_out) {
    if (!vfs || !inner_path || !file_out)
        return TCF_ERR_ARG;

    std::string wanted = normalize_inner_path(inner_path);
    if (!is_safe_inner_path(wanted))
        return TCF_ERR_ARG;

    // Find the entry
    auto it = std::find_if(vfs->entries.begin(), vfs->entries.end(),
                           [&wanted](const TCFEntry& e) { return e.path == wanted; });
    if (it == vfs->entries.end())
        return TCF_ERR_NOT_FOUND;

    // Validate offsets against payload size
    uint64_t payload_size = vfs->index_offset - vfs->data_offset;
    if (it->offset > payload_size || it->size > payload_size || it->offset + it->size > payload_size)
        return TCF_ERR_FORMAT;

    tcf_file_t *file = new tcf_file_t();
    file->vfs = vfs;
    file->offset = it->offset;
    file->size = it->size;
    file->position = 0;

    *file_out = file;
    return TCF_OK;
}

/**
 * Reads up to `size` bytes from a file into the provided buffer.
 * @param file The file handle.
 * @param buffer Destination buffer.
 * @param size Number of bytes to read.
 * @param bytes_read Actual number of bytes read (can be NULL).
 * @return TCF_OK on success (including EOF), error code otherwise.
 */
int tcf_vfs_read(tcf_file_t *file, void *buffer, size_t size, size_t *bytes_read) {
    if (!file || !buffer)
        return TCF_ERR_ARG;

    if (bytes_read)
        *bytes_read = 0;

    if (file->position >= file->size) {
        // EOF
        return TCF_OK;
    }

    uint64_t remaining64 = file->size - file->position;
    uint64_t to_read64 = std::min<uint64_t>(remaining64, static_cast<uint64_t>(size));
    size_t to_read = static_cast<size_t>(to_read64);
    if (to_read == 0)
        return TCF_OK;

    // Seek to the correct position in the archive
    uint64_t abs_off = file->vfs->data_offset + file->offset + file->position;
    if (abs_off > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()))
        return TCF_ERR_FORMAT;

    std::streamoff seek_pos = static_cast<std::streamoff>(abs_off);
    file->vfs->file.seekg(seek_pos, std::ios::beg);
    if (!file->vfs->file)
        return TCF_ERR_IO;

    // Read and unshift data in chunks (avoid per-call heap allocation)
    uint8_t temp_buffer[BUFFER_SIZE];
    size_t remaining = to_read;
    size_t written = 0;
    uint8_t *out = static_cast<uint8_t*>(buffer);

    while (remaining > 0) {
        size_t chunk = std::min(remaining, static_cast<size_t>(BUFFER_SIZE));
        file->vfs->file.read(reinterpret_cast<char*>(temp_buffer), static_cast<std::streamsize>(chunk));
        if (!file->vfs->file || static_cast<size_t>(file->vfs->file.gcount()) != chunk) {
            // Partial read or error
            return TCF_ERR_IO;
        }

        for (size_t j = 0; j < chunk; ++j) {
            out[written + j] = unshift_table[temp_buffer[j]];
        }

        written += chunk;
        remaining -= chunk;
    }

    file->position += to_read64;
    if (bytes_read)
        *bytes_read = to_read;

    return TCF_OK;
}

/**
 * Seeks the read position within a file.
 * @param file The file handle.
 * @param offset Offset to seek.
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END.
 * @return TCF_OK on success, error code otherwise.
 */
int tcf_vfs_seek(tcf_file_t *file, int64_t offset, int whence) {
    if (!file)
        return TCF_ERR_ARG;

    if (file->size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
        return TCF_ERR_ARG;

    int64_t new_pos;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = static_cast<int64_t>(file->position) + offset;
            break;
        case SEEK_END:
            new_pos = static_cast<int64_t>(file->size) + offset;
            break;
        default:
            return TCF_ERR_ARG;
    }

    if (new_pos < 0 || new_pos > static_cast<int64_t>(file->size))
        return TCF_ERR_ARG;

    file->position = static_cast<uint64_t>(new_pos);
    return TCF_OK;
}

/**
 * Returns the current read position of the file.
 * @param file The file handle.
 * @return Current position, or -1 on error.
 */
int64_t tcf_vfs_tell(tcf_file_t *file) {
    if (!file)
        return -1;
    if (file->position > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
        return -1;
    return static_cast<int64_t>(file->position);
}

/**
 * Closes a file handle and releases its resources.
 * @param file The file handle to close (can be NULL).
 */
void tcf_vfs_close_file(tcf_file_t *file) {
    delete file;
}

uint64_t tcf_vfs_entry_count(const tcf_vfs_t *vfs) {
    if (!vfs)
        return 0;
    return static_cast<uint64_t>(vfs->entries.size());
}

int tcf_vfs_entry_info(const tcf_vfs_t *vfs,
                       uint32_t index,
                       const char **path_out,
                       uint32_t *path_len_out,
                       uint64_t *size_out) {
    if (!vfs)
        return TCF_ERR_ARG;
    if (index >= vfs->entries.size())
        return TCF_ERR_ARG;

    const TCFEntry &entry = vfs->entries[index];
    if (path_out)
        *path_out = entry.path.c_str();
    if (path_len_out)
        *path_len_out = static_cast<uint32_t>(entry.path.size());
    if (size_out)
        *size_out = entry.size;

    return TCF_OK;
}

int tcf_vfs_read_entry(tcf_vfs_t *vfs, uint32_t index, void *buffer, size_t buffer_size) {
    if (!vfs || !buffer)
        return TCF_ERR_ARG;
    if (index >= vfs->entries.size())
        return TCF_ERR_ARG;

    const TCFEntry &entry = vfs->entries[index];
    if (entry.size > buffer_size)
        return TCF_ERR_ARG;

    uint64_t payload_size = vfs->index_offset - vfs->data_offset;
    if (entry.offset > payload_size || entry.size > payload_size || entry.offset + entry.size > payload_size)
        return TCF_ERR_FORMAT;

    uint64_t abs_off = vfs->data_offset + entry.offset;
    if (abs_off > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()))
        return TCF_ERR_FORMAT;

    std::streamoff seek_pos = static_cast<std::streamoff>(abs_off);
    vfs->file.seekg(seek_pos, std::ios::beg);
    if (!vfs->file)
        return TCF_ERR_IO;

    uint8_t temp_buffer[BUFFER_SIZE];
    uint64_t remaining = entry.size;
    size_t written = 0;
    uint8_t *out = static_cast<uint8_t*>(buffer);

    while (remaining > 0) {
        size_t chunk = static_cast<size_t>(std::min<uint64_t>(remaining, static_cast<uint64_t>(BUFFER_SIZE)));
        vfs->file.read(reinterpret_cast<char*>(temp_buffer), static_cast<std::streamsize>(chunk));
        if (!vfs->file || static_cast<size_t>(vfs->file.gcount()) != chunk)
            return TCF_ERR_IO;

        for (size_t j = 0; j < chunk; ++j) {
            out[written + j] = unshift_table[temp_buffer[j]];
        }

        written += chunk;
        remaining -= chunk;
    }

    return TCF_OK;
}
