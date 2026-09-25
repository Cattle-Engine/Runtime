#pragma once

#include <string>
#include "engine/common/fs/tdf.hpp"

namespace CE::Scripting::Bindings::TDF {
    inline CE::TDF::Value MakeNull() { return CE::TDF::File::makeNull(); }
    inline CE::TDF::Value MakeBool(bool value) { return CE::TDF::File::makeBool(value); }
    inline CE::TDF::Value MakeInt(int value) { return CE::TDF::File::makeInt(value); }
    inline CE::TDF::Value MakeUInt(uint32_t value) { return CE::TDF::File::makeUInt(value); }
    inline CE::TDF::Value MakeFloat(float value) { return CE::TDF::File::makeFloat(value); }
    inline CE::TDF::Value MakeString(const std::string& value) { return CE::TDF::File::makeString(value); }
    inline bool ReadBool(const CE::TDF::Value& value) { try { return CE::TDF::File::readBool(value); } catch (...) { return false; } }
    inline int ReadInt(const CE::TDF::Value& value) { try { return CE::TDF::File::readInt(value); } catch (...) { return 0; } }
    inline uint32_t ReadUInt(const CE::TDF::Value& value) { try { return CE::TDF::File::readUInt(value); } catch (...) { return 0; } }
    inline float ReadFloat(const CE::TDF::Value& value) { try { return CE::TDF::File::readFloat(value); } catch (...) { return 0.0f; } }
    inline std::string ReadString(const CE::TDF::Value& value) { try { return CE::TDF::File::readString(value); } catch (...) { return {}; } }
    inline CE::TDF::Value Get(const CE::TDF::File& file, const std::string& key) { CE::TDF::Value value; file.tryGetPath(key, value); return value; }
} // namespace CE::Scripting::Bindings::TDF
