#include "ce_common/tdf.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <system_error>
#include <utility>

namespace CE {
    namespace {

        using Type = enum TDF::Type;

        constexpr char kMagic[3] = {'T', 'D', 'F'};
        constexpr size_t kReadChunk = 1u << 16;

        [[noreturn]] void Fail(const std::string& msg) {
            throw TDF::Error("TDF: " + msg);
        }

        // checksum
        uint32_t CrcUpdate(uint32_t crc, const uint8_t* p, size_t n) {
            static const std::array<uint32_t, 256> table = [] {
                std::array<uint32_t, 256> t{};
                for (uint32_t i = 0; i < 256; ++i) {
                    uint32_t c = i;
                    for (int k = 0; k < 8; ++k)
                        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                    t[i] = c;
                }
                return t;
            }();
            for (size_t i = 0; i < n; ++i)
                crc = table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
            return crc;
        }

        // zigzag
        uint32_t Zz32(int32_t n) {
            const auto u = static_cast<uint32_t>(n);
            return (u << 1) ^ (0u - (u >> 31));
        }
        int32_t Unzz32(uint32_t v) {
            return static_cast<int32_t>((v >> 1) ^ (0u - (v & 1u)));
        }
        uint64_t Zz64(int64_t n) {
            const auto u = static_cast<uint64_t>(n);
            return (u << 1) ^ (0ull - (u >> 63));
        }
        int64_t Unzz64(uint64_t v) {
            return static_cast<int64_t>((v >> 1) ^ (0ull - (v & 1ull)));
        }

        Type ToType(uint8_t raw) {
            if (raw > static_cast<uint8_t>(Type::Array))
                Fail("unknown Type tag " + std::to_string(raw));
            return static_cast<Type>(raw);
        }

        class Writer {
          public:
            explicit Writer(std::ostream& os) : mOs(os) {}

            void Raw(const void* p, size_t n) {
                if (n == 0)
                    return;
                mOs.write(static_cast<const char*>(p), static_cast<std::streamsize>(n));
                if (!mOs)
                    Fail("stream write failed");
                mCrc = CrcUpdate(mCrc, static_cast<const uint8_t*>(p), n);
            }
            void U8(uint8_t v) {
                Raw(&v, 1);
            }
            void Varint(uint64_t v) {
                uint8_t buf[10];
                size_t n = 0;
                while (v >= 0x80) {
                    buf[n++] = static_cast<uint8_t>((v & 0x7F) | 0x80);
                    v >>= 7;
                }
                buf[n++] = static_cast<uint8_t>(v);
                Raw(buf, n);
            }
            void Le32(uint32_t v) {
                const uint8_t b[4] = {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v >> 16), static_cast<uint8_t>(v >> 24)};
                Raw(b, 4);
            }
            void Le64(uint64_t v) {
                uint8_t b[8];
                for (int i = 0; i < 8; ++i)
                    b[i] = static_cast<uint8_t>(v >> (8 * i));
                Raw(b, 8);
            }
            void F32(float f) {
                uint32_t u;
                std::memcpy(&u, &f, 4);
                Le32(u);
            }
            void F64(double d) {
                uint64_t u;
                std::memcpy(&u, &d, 8);
                Le64(u);
            }
            void Str(std::string_view s) {
                Varint(s.size());
                Raw(s.data(), s.size());
            }

            void Finish() {
                const uint32_t c = ~mCrc;
                const uint8_t b[4] = {static_cast<uint8_t>(c), static_cast<uint8_t>(c >> 8), static_cast<uint8_t>(c >> 16), static_cast<uint8_t>(c >> 24)};
                mOs.write(reinterpret_cast<const char*>(b), 4);
                mOs.flush();
                if (!mOs)
                    Fail("stream write failed");
            }

          private:
            std::ostream& mOs;
            uint32_t mCrc = 0xFFFFFFFFu;
        };

        void WritePayload(Writer& w, const TDF& v, size_t depth) {
            switch (v.Type()) {
            case Type::Null:
                break;
            case Type::Bool:
                w.U8(v.Get<bool>() ? 1 : 0);
                break;
            case Type::Int32:
                w.Varint(Zz32(v.Get<int32_t>()));
                break;
            case Type::UInt32:
                w.Varint(v.Get<uint32_t>());
                break;
            case Type::Int64:
                w.Varint(Zz64(v.Get<int64_t>()));
                break;
            case Type::UInt64:
                w.Varint(v.Get<uint64_t>());
                break;
            case Type::Float:
                w.F32(v.Get<float>());
                break;
            case Type::Double:
                w.F64(v.Get<double>());
                break;
            case Type::String:
                w.Str(v.Get<std::string>());
                break;
            case Type::Bytes: {
                const auto& b = v.Get<TDF::Bytes>();
                w.Varint(b.size());
                w.Raw(b.data(), b.size());
                break;
            }
            case Type::Object: {
                if (depth >= TDF::kDefaultMaxDepth)
                    Fail("nesting too deep");
                const auto& o = v.Get<TDF::Object>();
                w.Varint(o.size());
                for (const auto& [key, child] : o) {
                    w.Str(key);
                    w.U8(static_cast<uint8_t>(child.Type()));
                    WritePayload(w, child, depth + 1);
                }
                break;
            }
            case Type::Array: {
                if (depth >= TDF::kDefaultMaxDepth)
                    Fail("nesting too deep");
                const auto& a = v.Get<TDF::Array>();
                const Type et = a.empty() ? Type::Null : a.front().Type();
                if (et == Type::Null && !a.empty())
                    Fail("arrays cannot contain Null");
                for (const auto& e : a) {
                    if (e.Type() != et)
                        Fail("array elements must share one Type");
                }
                w.U8(static_cast<uint8_t>(et));
                w.Varint(a.size());
                for (const auto& e : a)
                    WritePayload(w, e, depth + 1);
                break;
            }
            }
        }

        // reader
        class Reader {
          public:
            explicit Reader(std::istream& is) : mIs(is) {}

            void Raw(void* p, size_t n) {
                RawNoCrc(p, n);
                mCrc = CrcUpdate(mCrc, static_cast<const uint8_t*>(p), n);
            }
            uint8_t U8() {
                uint8_t v;
                Raw(&v, 1);
                return v;
            }
            uint64_t Varint() {
                uint64_t r = 0;
                for (unsigned i = 0; i < 10; ++i) {
                    const uint8_t b = U8();
                    if (i == 9 && b > 1)
                        Fail("varint overflow");
                    r |= static_cast<uint64_t>(b & 0x7F) << (7 * i);
                    if (!(b & 0x80))
                        return r;
                }
                Fail("varint too long");
            }
            uint32_t Le32() {
                uint8_t b[4];
                Raw(b, 4);
                return static_cast<uint32_t>(b[0]) | static_cast<uint32_t>(b[1]) << 8 | static_cast<uint32_t>(b[2]) << 16 | static_cast<uint32_t>(b[3]) << 24;
            }
            uint64_t Le64() {
                uint8_t b[8];
                Raw(b, 8);
                uint64_t v = 0;
                for (int i = 0; i < 8; ++i)
                    v |= static_cast<uint64_t>(b[i]) << (8 * i);
                return v;
            }
            float F32() {
                const uint32_t u = Le32();
                float f;
                std::memcpy(&f, &u, 4);
                return f;
            }
            double F64() {
                const uint64_t u = Le64();
                double d;
                std::memcpy(&d, &u, 8);
                return d;
            }

            // Reads n bytes in bounded chunks so a lying length field cannot force a huge allocation.
            template <class Container> Container Chunked(uint64_t n) {
                Container out;
                uint64_t done = 0;
                while (done < n) {
                    const size_t chunk = static_cast<size_t>(std::min<uint64_t>(n - done, kReadChunk));
                    out.resize(static_cast<size_t>(done) + chunk);
                    Raw(out.data() + done, chunk);
                    done += chunk;
                }
                return out;
            }

            uint32_t ExpectedCrc() const {
                return ~mCrc;
            }
            uint32_t Trailer() {
                uint8_t b[4];
                RawNoCrc(b, 4);
                return static_cast<uint32_t>(b[0]) | static_cast<uint32_t>(b[1]) << 8 | static_cast<uint32_t>(b[2]) << 16 | static_cast<uint32_t>(b[3]) << 24;
            }

          private:
            void RawNoCrc(void* p, size_t n) {
                if (n == 0)
                    return;
                mIs.read(static_cast<char*>(p), static_cast<std::streamsize>(n));
                if (std::cmp_not_equal(mIs.gcount(), n))
                    Fail("unexpected end of stream");
            }

            std::istream& mIs;
            uint32_t mCrc = 0xFFFFFFFFu;
        };

        TDF ReadPayload(Reader& r, Type t, size_t depth, size_t maxDepth) {
            switch (t) {
            case Type::Null:
                return TDF();
            case Type::Bool: {
                const uint8_t b = r.U8();
                if (b > 1)
                    Fail("invalid bool");
                return TDF(b == 1);
            }
            case Type::Int32: {
                const uint64_t v = r.Varint();
                if (v > 0xFFFFFFFFull)
                    Fail("Int32 out of range");
                return TDF(Unzz32(static_cast<uint32_t>(v)));
            }
            case Type::UInt32: {
                const uint64_t v = r.Varint();
                if (v > 0xFFFFFFFFull)
                    Fail("UInt32 out of range");
                return TDF(static_cast<uint32_t>(v));
            }
            case Type::Int64:
                return TDF(Unzz64(r.Varint()));
            case Type::UInt64:
                return TDF(r.Varint());
            case Type::Float:
                return TDF(r.F32());
            case Type::Double:
                return TDF(r.F64());
            case Type::String:
                return TDF(r.Chunked<std::string>(r.Varint()));
            case Type::Bytes:
                return TDF(r.Chunked<TDF::Bytes>(r.Varint()));
            case Type::Object: {
                if (depth >= maxDepth)
                    Fail("nesting too deep");
                const uint64_t count = r.Varint();
                TDF::Object obj;
                for (uint64_t i = 0; i < count; ++i) {
                    auto key = r.Chunked<std::string>(r.Varint());
                    const Type ct = ToType(r.U8());
                    TDF child = ReadPayload(r, ct, depth + 1, maxDepth);
                    if (!obj.emplace(std::move(key), std::move(child)).second)
                        Fail("duplicate key");
                }
                return TDF(std::move(obj));
            }
            case Type::Array: {
                if (depth >= maxDepth)
                    Fail("nesting too deep");
                const Type et = ToType(r.U8());
                const uint64_t count = r.Varint();
                // Every non-Null element consumes >= 1 byte, so `count` is bounded by the input.
                if (et == Type::Null && count > 0)
                    Fail("arrays cannot contain Null");
                TDF::Array arr;
                arr.reserve(static_cast<size_t>(std::min<uint64_t>(count, 4096)));
                for (uint64_t i = 0; i < count; ++i)
                    arr.push_back(ReadPayload(r, et, depth + 1, maxDepth));
                return TDF(std::move(arr));
            }
            }
            Fail("unknown Type");
        }

        // misc
        struct MemBuf : std::streambuf {
            MemBuf(const uint8_t* d, size_t n) {
                char* b = const_cast<char*>(reinterpret_cast<const char*>(d));
                setg(b, b, b + n);
            }
        };

        std::vector<std::string_view> SplitPath(std::string_view path, char sep) {
            std::vector<std::string_view> parts;
            size_t pos = 0;
            while (true) {
                const size_t e = path.find(sep, pos);
                const std::string_view seg = path.substr(pos, e == std::string_view::npos ? e : e - pos);
                if (seg.empty())
                    Fail("empty path segment");
                parts.push_back(seg);
                if (e == std::string_view::npos)
                    break;
                pos = e + 1;
            }
            return parts;
        }

    } // namespace

    const char* TDF::TypeName(enum Type t) noexcept {
        static constexpr const char* kNames[] = {"Null",  "Bool",   "Int32",  "UInt32", "Int64",  "UInt64",
                                                "Float", "Double", "String", "Bytes",  "Object", "Array",};
        const auto i = static_cast<size_t>(t);
        return i < std::size(kNames) ? kNames[i] : "Unknown";
    }

    void TDF::TypeMismatch() const {
        Fail(std::string("Type mismatch, value is ") + TypeName(Type()));
    }

    // container
    size_t TDF::Size() const {
        if (const auto* o = std::get_if<Object>(&mV))
            return o->size();
        if (const auto* a = std::get_if<Array>(&mV))
            return a->size();
        Fail(std::string("Size() requires Object or Array, value is ") + TypeName(Type()));
    }

    TDF& TDF::operator[](std::string_view key) {
        if (IsNull())
            mV.emplace<Object>();
        auto* o = std::get_if<Object>(&mV);
        if (!o)
            Fail(std::string("operator[](key) requires Object, value is ") + TypeName(Type()));
        auto it = o->find(key);
        if (it == o->end())
            it = o->emplace(std::string(key), TDF()).first;
        return it->second;
    }

    const TDF* TDF::Find(std::string_view key) const {
        const auto* o = std::get_if<Object>(&mV);
        if (!o)
            return nullptr;
        const auto it = o->find(key);
        return it == o->end() ? nullptr : &it->second;
    }

    TDF* TDF::Find(std::string_view key) {
        return const_cast<TDF*>(std::as_const(*this).Find(key));
    }

    const TDF& TDF::At(std::string_view key) const {
        if (const TDF* p = Find(key))
            return *p;
        Fail("key not found: " + std::string(key));
    }

    TDF& TDF::At(std::string_view key) {
        return const_cast<TDF&>(std::as_const(*this).At(key));
    }

    bool TDF::Erase(std::string_view key) {
        auto* o = std::get_if<Object>(&mV);
        if (!o)
            return false;
        const auto it = o->find(key);
        if (it == o->end())
            return false;
        o->erase(it);
        return true;
    }

    const TDF& TDF::At(size_t index) const {
        const auto* a = std::get_if<Array>(&mV);
        if (!a)
            Fail(std::string("at(index) requires Array, value is ") + TypeName(Type()));
        if (index >= a->size())
            Fail("array index out of range");
        return (*a)[index];
    }

    TDF& TDF::At(size_t index) {
        return const_cast<TDF&>(std::as_const(*this).At(index));
    }

    void TDF::PushBack(TDF val) {
        if (IsNull())
            mV.emplace<Array>();
        auto* a = std::get_if<Array>(&mV);
        if (!a)
            Fail(std::string("push_back requires Array, value is ") + TypeName(Type()));
        if (val.IsNull())
            Fail("arrays cannot contain Null");
        if (!a->empty() && a->front().Type() != val.Type())
            Fail("array elements must share one Type");
        a->push_back(std::move(val));
    }

    void TDF::Erase(size_t index) {
        auto* a = std::get_if<Array>(&mV);
        if (!a)
            Fail(std::string("erase(index) requires Array, value is ") + TypeName(Type()));
        if (index >= a->size())
            Fail("array index out of range");
        a->erase(a->begin() + static_cast<std::ptrdiff_t>(index));
    }

    // paths
    const TDF* TDF::FindPath(std::string_view path, char sep) const {
        const TDF* cur = this;
        for (const auto seg : SplitPath(path, sep)) {
            cur = cur->Find(seg);
            if (!cur)
                return nullptr;
        }
        return cur;
    }

    TDF* TDF::FindPath(std::string_view path, char sep) {
        return const_cast<TDF*>(std::as_const(*this).FindPath(path, sep));
    }

    TDF& TDF::SetPath(std::string_view path, TDF val, char sep) {
        const auto parts = SplitPath(path, sep);
        TDF* cur = this;
        for (size_t i = 0; i + 1 < parts.size(); ++i)
            cur = &(*cur)[parts[i]];
        TDF& slot = (*cur)[parts.back()];
        slot = std::move(val);
        return slot;
    }

    bool TDF::ErasePath(std::string_view path, char sep) {
        const auto parts = SplitPath(path, sep);
        TDF* cur = this;
        for (size_t i = 0; i + 1 < parts.size(); ++i) {
            cur = cur->Find(parts[i]);
            if (!cur)
                return false;
        }
        return cur->Erase(parts.back());
    }

    // serialisation
    void TDF::Write(std::ostream& os) const {
        Writer w(os);
        w.Raw(kMagic, 3);
        w.U8(kVersion);
        w.U8(static_cast<uint8_t>(Type()));
        WritePayload(w, *this, 0);
        w.Finish();
    }

    TDF TDF::Read(std::istream& is, size_t maxDepth) {
        Reader r(is);

        char magic[3];
        r.Raw(magic, 3);
        if (std::memcmp(magic, kMagic, 3) != 0)
            Fail("invalid magic");

        const uint8_t version = r.U8();
        if (version != kVersion)
            Fail("unsupported version " + std::to_string(version));

        const enum Type root_type = ToType(r.U8());
        TDF root = ReadPayload(r, root_type, 0, maxDepth);

        const uint32_t expected = r.ExpectedCrc();
        if (r.Trailer() != expected)
            Fail("checksum mismatch");
        return root;
    }

    TDF::Bytes TDF::ToBytes() const {
        std::ostringstream os(std::ios::binary);
        Write(os);
        const std::string s = os.str();
        return Bytes(s.begin(), s.end());
    }

    TDF TDF::FromBytes(const uint8_t* data, size_t Size, size_t maxDepth) {
        MemBuf buf(data, Size);
        std::istream is(&buf);
        return Read(is, maxDepth);
    }

    void TDF::Save(const std::filesystem::path& path) const {
        std::filesystem::path tmp = path;
        tmp += ".tmp";
        try {
            {
                std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
                if (!out)
                    Fail("cannot open for writing: " + tmp.string());
                Write(out);
                out.close();
                if (!out)
                    Fail("incomplete write");
            }
            std::filesystem::rename(tmp, path);
        } catch (...) {
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
            throw;
        }
    }

    TDF TDF::Load(const std::filesystem::path& path, size_t maxDepth) {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            Fail("cannot open for reading: " + path.string());
        return Read(in, maxDepth);
    }
} // namespace CE