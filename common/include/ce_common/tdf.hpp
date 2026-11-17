#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace CE {
    class TDF {
      public:
        enum class Type : uint8_t {
            Null = 0,
            Bool,
            Int32,
            UInt32,
            Int64,
            UInt64,
            Float,
            Double,
            String,
            Bytes,
            Object,
            Array,
        };

        using Bytes = std::vector<uint8_t>;
        using Object = std::map<std::string, TDF, std::less<>>;
        using Array = std::vector<TDF>;

        class Error : public std::runtime_error {
          public:
            using std::runtime_error::runtime_error;
        };

        static constexpr uint8_t kVersion = 0x20;
        static constexpr size_t kDefaultMaxDepth = 64;

        TDF() = default;
        TDF(std::nullptr_t) {}
        TDF(bool v) : mV(std::in_place_type<bool>, v) {}
        TDF(int32_t v) : mV(std::in_place_type<int32_t>, v) {}
        TDF(uint32_t v) : mV(std::in_place_type<uint32_t>, v) {}
        TDF(int64_t v) : mV(std::in_place_type<int64_t>, v) {}
        TDF(uint64_t v) : mV(std::in_place_type<uint64_t>, v) {}
        TDF(float v) : mV(std::in_place_type<float>, v) {}
        TDF(double v) : mV(std::in_place_type<double>, v) {}
        TDF(std::string v) : mV(std::in_place_type<std::string>, std::move(v)) {}
        TDF(const char* v) : mV(std::in_place_type<std::string>, v) {}
        TDF(Bytes v) : mV(std::in_place_type<Bytes>, std::move(v)) {}
        TDF(Object v) : mV(std::in_place_type<Object>, std::move(v)) {}
        TDF(Array v) : mV(std::in_place_type<Array>, std::move(v)) {}

        static TDF MakeObject() {
            return TDF(Object{});
        }
        static TDF MakeArray() {
            return TDF(Array{});
        }
        static TDF MakeObject(std::initializer_list<std::pair<const std::string, TDF>> init) {
            return TDF(Object(init));
        }
        static TDF MakeArray(std::initializer_list<TDF> init) {
            return TDF(Array(init));
        }

        // type inspection
        Type Type() const noexcept {
            return static_cast<enum Type>(mV.index());
        }
        static const char* TypeName(enum Type t) noexcept;

        bool IsNull() const noexcept {
            return Type() == Type::Null;
        }
        bool IsObject() const noexcept {
            return Type() == Type::Object;
        }
        bool IsArray() const noexcept {
            return Type() == Type::Array;
        }
        bool IsString() const noexcept {
            return Type() == Type::String;
        }

        // typed access (exact type; throws Error on mismatch)
        // T is one of: bool, int32_t, uint32_t, int64_t, uint64_t, float, double,
        //              std::string, Bytes, Object, Array
        template <class T> T& Get() {
            if (auto* p = std::get_if<T>(&mV))
                return *p;
            TypeMismatch();
        }
        template <class T> const T& Get() const {
            if (auto* p = std::get_if<T>(&mV))
                return *p;
            TypeMismatch();
        }
        template <class T> T* TryGet() noexcept {
            return std::get_if<T>(&mV);
        }
        template <class T> const T* TryGet() const noexcept {
            return std::get_if<T>(&mV);
        }

        // object member of type T, or fallback if missing / wrong type.
        template <class T> T Value(std::string_view key, T fallback) const {
            if (const TDF* c = Find(key)) {
                if (const T* p = c->TryGet<T>())
                    return *p;
            }
            return fallback;
        }

        // containers
        size_t Size() const; // Object or Array
        bool Empty() const {
            return Size() == 0;
        }

        // Object
        TDF& operator[](std::string_view key); // Null -> Object; inserts if missing
        TDF& At(std::string_view key);
        const TDF& At(std::string_view key) const;
        TDF* Find(std::string_view key);
        const TDF* Find(std::string_view key) const;
        bool Contains(std::string_view key) const {
            return Find(key) != nullptr;
        }
        bool Erase(std::string_view key);

        // array
        TDF& operator[](size_t index) {
            return At(index);
        }
        TDF& At(size_t index);
        const TDF& At(size_t index) const;
        void PushBack(TDF val); // Null -> Array; elements must share one type
        void Erase(size_t index);

        // paths ("a/b/c" through nested Objects)
        const TDF* FindPath(std::string_view path, char sep = '/') const;
        TDF* FindPath(std::string_view path, char sep = '/');
        bool HasPath(std::string_view path, char sep = '/') const {
            return FindPath(path, sep) != nullptr;
        }
        TDF& SetPath(std::string_view path, TDF val, char sep = '/'); // creates intermediate Objects
        bool ErasePath(std::string_view path, char sep = '/');

        // Streams must be opened in binary mode
        void Write(std::ostream& os) const;
        static TDF Read(std::istream& is, size_t maxDepth = kDefaultMaxDepth);

        Bytes ToBytes() const;
        static TDF FromBytes(const uint8_t* data, size_t size, size_t maxDepth = kDefaultMaxDepth);
        static TDF FromBytes(const Bytes& b, size_t maxDepth = kDefaultMaxDepth) {
            return FromBytes(b.data(), b.size(), maxDepth);
        }

        // save() writes to "<path>.tmp" then renames, so a crash never leaves a half-written file.
        void Save(const std::filesystem::path& path) const;
        static TDF Load(const std::filesystem::path& path, size_t maxDepth = kDefaultMaxDepth);

        friend bool operator==(const TDF& a, const TDF& b) {
            return a.mV == b.mV;
        }
        friend bool operator!=(const TDF& a, const TDF& b) {
            return !(a == b);
        }

      private:
        using Variant = std::variant<std::monostate, bool, int32_t, uint32_t, int64_t, uint64_t, float, double,
                                     std::string, Bytes, Object, Array>; // order == Type enum
        [[noreturn]] void TypeMismatch() const;

        Variant mV;
    };

} // namespace CE