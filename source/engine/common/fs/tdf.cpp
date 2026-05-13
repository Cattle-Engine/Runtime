#include "engine/common/fs/tdf.hpp"
#include "engine/common/fs/vfs.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <tuple>

namespace {

constexpr uint8_t kMagic[3] = { 'T', 'D', 'F' };

[[noreturn]] void throwFormat(const char* msg) {
    throw std::runtime_error(std::string("TDF: ") + msg);
}

void writeU32(std::vector<uint8_t>& out, uint32_t v) {
    uint8_t b[4];
    std::memcpy(b, &v, 4);
    out.insert(out.end(), b, b + 4);
}

uint32_t readU32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

std::vector<std::string> splitPath(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::string cur;

    for (char c : s) {
        if (c == sep) {
            if (cur.empty()) throwFormat("empty path segment");
            parts.push_back(std::move(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }

    if (cur.empty()) throwFormat("empty path segment");
    parts.push_back(std::move(cur));
    return parts;
}

std::vector<uint8_t> serializeToBytes(const TDFFile& file, uint8_t version) {
    std::vector<uint8_t> bytes;
    bytes.insert(bytes.end(), kMagic, kMagic + 3);
    bytes.push_back(version);

    std::vector<uint8_t> index;
    std::vector<uint8_t> data;

    for (const auto& [key, val] : file.entries) {
        if (key.empty() || key.size() > 255) throwFormat("key length must be 1..255");
        if (version < 0x11 && (val.type == TDFType::Object || val.type == TDFType::ArrObject))
            throwFormat("Object/ArrObject requires version >= 0x11");

        uint32_t offset = static_cast<uint32_t>(data.size());

        index.push_back(static_cast<uint8_t>(key.size()));
        index.insert(index.end(), key.begin(), key.end());
        index.push_back(static_cast<uint8_t>(val.type));
        writeU32(index, offset);

        data.insert(data.end(), val.data.begin(), val.data.end());
    }

    index.push_back(0x00);

    bytes.insert(bytes.end(), index.begin(), index.end());
    bytes.insert(bytes.end(), data.begin(), data.end());
    return bytes;
}

TDFFile parseFromBytes(const uint8_t* data, size_t size) {
    if (size < 4) throwFormat("buffer too small");
    if (data[0] != kMagic[0] || data[1] != kMagic[1] || data[2] != kMagic[2]) throwFormat("invalid magic");

    uint8_t version = data[3];
    size_t pos = 4;

    std::vector<std::tuple<std::string, TDFType, uint32_t>> index;

    while (true) {
        if (pos >= size) throwFormat("unexpected EOF in index");

        uint8_t keyLen = data[pos++];
        if (keyLen == 0) break;

        if (pos + keyLen + 1 + 4 > size) throwFormat("index overflow");

        std::string key(
            reinterpret_cast<const char*>(data + pos),
            reinterpret_cast<const char*>(data + pos + keyLen)
        );
        pos += keyLen;

        TDFType type = static_cast<TDFType>(data[pos++]);
        uint32_t offset = readU32(data + pos);
        pos += 4;

        index.emplace_back(std::move(key), type, offset);
    }

    size_t dataStart = pos;

    TDFFile file;

    for (auto& [key, type, offset] : index) {
        size_t p = dataStart + offset;
        if (p > size) throwFormat("offset out of range");

        TDFValue v{ type, {} };

        auto require = [&](size_t n) {
            if (p + n > size) throwFormat("unexpected EOF in value");
        };

        auto isArray = [](TDFType t) {
            uint8_t v = static_cast<uint8_t>(t);
            return v >= 0xE0 && v <= 0xE5;
        };

        if (v.type == TDFType::Null) {
        }
        else if (v.type == TDFType::String) {
            do {
                require(1);
                uint8_t c = data[p++];
                v.data.push_back(c);
                if (c == 0) break;
            } while (true);
        }
        else if (v.type == TDFType::Object) {
            require(4);
            uint32_t len = readU32(data + p);
            require(4 + len);
            v.data.resize(4 + len);
            std::memcpy(v.data.data(), data + p, 4 + len);
            p += 4 + len;
        }
        else if (isArray(v.type)) {
            require(4);
            uint32_t count = readU32(data + p);

            if (v.type == TDFType::ArrString) {
                v.data.insert(v.data.end(), data + p, data + p + 4);
                p += 4;

                for (uint32_t i = 0; i < count; i++) {
                    while (true) {
                        require(1);
                        uint8_t c = data[p++];
                        v.data.push_back(c);
                        if (c == 0) break;
                    }
                }
            }
            else if (v.type == TDFType::ArrObject) {
                v.data.insert(v.data.end(), data + p, data + p + 4);
                p += 4;

                for (uint32_t i = 0; i < count; i++) {
                    require(4);
                    uint32_t len = readU32(data + p);
                    require(4 + len);

                    v.data.insert(v.data.end(), data + p, data + p + 4 + len);
                    p += 4 + len;
                }
            }
            else {
                size_t elemSize = 0;
                switch (v.type) {
                    case TDFType::ArrBool: elemSize = 1; break;
                    case TDFType::ArrInt32:
                    case TDFType::ArrUInt32:
                    case TDFType::ArrFloat: elemSize = 4; break;
                    default: throwFormat("unknown array type");
                }

                require(4 + elemSize * count);
                v.data.resize(4 + elemSize * count);
                std::memcpy(v.data.data(), data + p, v.data.size());
                p += v.data.size();
            }
        }
        else {
            size_t sz = 0;
            switch (v.type) {
                case TDFType::Bool: sz = 1; break;
                case TDFType::Int32:
                case TDFType::UInt32:
                case TDFType::Float: sz = 4; break;
                default: throwFormat("unknown scalar");
            }

            require(sz);
            v.data.resize(sz);
            std::memcpy(v.data.data(), data + p, sz);
            p += sz;
        }

        file.entries.emplace(std::move(key), std::move(v));
    }

    return file;
}

TDFFile decodeObjectValue(const TDFValue& v) {
    if (v.type != TDFType::Object) throwFormat("value is not Object");
    if (v.data.size() < 4) throwFormat("object too small");

    uint32_t len = readU32(v.data.data());
    if (v.data.size() != 4ull + len) throwFormat("object size mismatch");

    return parseFromBytes(v.data.data() + 4, len);
}

TDFValue encodeObjectValue(const TDFFile& obj, uint8_t version) {
    std::vector<uint8_t> blob = serializeToBytes(obj, version);
    TDFValue v{ TDFType::Object, {} };
    writeU32(v.data, static_cast<uint32_t>(blob.size()));
    v.data.insert(v.data.end(), blob.begin(), blob.end());
    return v;
}

std::vector<std::string> decodeStringArray(const TDFValue& v) {
    if (v.type != TDFType::ArrString) throwFormat("not ArrString");
    if (v.data.size() < 4) throwFormat("ArrString too small");

    uint32_t count = readU32(v.data.data());
    std::vector<std::string> out;
    out.reserve(count);

    size_t p = 4;

    for (uint32_t i = 0; i < count; i++) {
        std::string s;
        while (true) {
            if (p >= v.data.size()) throwFormat("ArrString missing terminator");
            uint8_t c = v.data[p++];
            if (c == 0) break;
            s.push_back((char)c);
        }
        out.push_back(std::move(s));
    }

    return out;
}

TDFValue encodeStringArray(const std::vector<std::string>& arr) {
    TDFValue v{ TDFType::ArrString, {} };
    writeU32(v.data, (uint32_t)arr.size());

    for (auto& s : arr) {
        v.data.insert(v.data.end(), s.begin(), s.end());
        v.data.push_back(0);
    }

    return v;
}

std::vector<TDFFile> decodeObjectArray(const TDFValue& v) {
    if (v.type != TDFType::ArrObject) throwFormat("not ArrObject");
    if (v.data.size() < 4) throwFormat("ArrObject too small");

    uint32_t count = readU32(v.data.data());

    std::vector<TDFFile> out;
    out.reserve(count);

    size_t p = 4;

    for (uint32_t i = 0; i < count; i++) {
        if (p + 4 > v.data.size()) throwFormat("ArrObject truncated");

        uint32_t len = readU32(v.data.data() + p);
        p += 4;

        if (p + len > v.data.size()) throwFormat("ArrObject overflow");

        out.push_back(parseFromBytes(v.data.data() + p, len));
        p += len;
    }

    return out;
}

TDFValue encodeObjectArray(const std::vector<TDFFile>& arr, uint8_t version) {
    TDFValue v{ TDFType::ArrObject, {} };
    writeU32(v.data, (uint32_t)arr.size());

    for (auto& obj : arr) {
        auto blob = serializeToBytes(obj, version);
        writeU32(v.data, (uint32_t)blob.size());
        v.data.insert(v.data.end(), blob.begin(), blob.end());
    }

    return v;
}

} // namespace

void TDFFile::save(CE::VFS::VFS& vfs, const std::string& path, uint8_t version) const {
    VirtualFile* f = vfs.V_fopen(path.c_str(), "wb");
    if (!f || !f->sdl_stream) throw std::runtime_error("TDF: VFS write fail");

    auto bytes = serializeToBytes(*this, version);

    size_t written = SDL_WriteIO(f->sdl_stream, bytes.data(), bytes.size());

    vfs.V_fclose(f);

    if (written != bytes.size()) throw std::runtime_error("TDF: incomplete write");
}

void TDFFile::load(CE::VFS::VFS& vfs, const std::string& path) {
    VirtualFile* f = vfs.OpenFile(path.c_str());
    if (!f) throw std::runtime_error("TDF: VFS open fail");

    size_t size = (size_t)f->size;
    std::vector<uint8_t> buffer(size);

    size_t read = vfs.ReadFile(f, buffer.data(), buffer.size());
    vfs.CloseFile(f);

    if (read != buffer.size()) throw std::runtime_error("TDF: incomplete read");

    *this = parseFromBytes(buffer.data(), buffer.size());
}

void TDFFile::set(const std::string& key, const TDFValue& val) {
    entries[key] = val;
}

bool TDFFile::remove(const std::string& key) {
    return entries.erase(key) > 0;
}

bool TDFFile::has(const std::string& key) const {
    return entries.find(key) != entries.end();
}