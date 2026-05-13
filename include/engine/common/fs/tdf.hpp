#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>

#include "engine/common/fs/vfs.hpp"

enum class TDFType : uint8_t {
    Null = 0,
    Bool,
    Int32,
    UInt32,
    Float,
    String,
    Object,

    ArrBool = 0xE0,
    ArrInt32,
    ArrUInt32,
    ArrFloat,
    ArrString,
    ArrObject,
};

struct TDFValue {
    TDFType type;
    std::vector<uint8_t> data;
};

struct TDFFile {
    std::unordered_map<std::string, TDFValue> entries;

    void save(CE::VFS::VFS& vfs, const std::string& path, uint8_t version) const;
    void load(CE::VFS::VFS& vfs, const std::string& path);

    void set(const std::string& key, const TDFValue& val);
    bool remove(const std::string& key);
    bool has(const std::string& key) const;

    bool hasPath(const std::string& path, char separator = '/');
    bool tryGetPath(const std::string& path, TDFValue& out, char separator = '/');
    bool removePath(const std::string& path, char separator = '/');
    void setPath(const std::string& path, const TDFValue& val, char separator = '/');

    void appendToArray(const std::string& key, const TDFValue& val);
    void deleteFromArray(const std::string& key, size_t index);

    static TDFValue makeNull();
    static TDFValue makeBool(bool v);
    static TDFValue makeInt(int32_t v);
    static TDFValue makeUInt(uint32_t v);
    static TDFValue makeFloat(float v);
    static TDFValue makeString(const std::string& s);

    static TDFValue makeBoolArray(const std::vector<bool>& arr);
    static TDFValue makeIntArray(const std::vector<int32_t>& arr);
    static TDFValue makeUIntArray(const std::vector<uint32_t>& arr);
    static TDFValue makeFloatArray(const std::vector<float>& arr);
    static TDFValue makeStringArray(const std::vector<std::string>& arr);

    static TDFValue makeObject(const TDFFile& obj, uint8_t version);
    static TDFValue makeObjectArray(const std::vector<TDFFile>& arr, uint8_t version);

    static void readValue(VirtualFile* file, TDFValue& v);

    static bool isArray(TDFType t);
    static TDFType elementType(TDFType arrType);
    static size_t elementSize(TDFType arrType, TDFType elemType);
};