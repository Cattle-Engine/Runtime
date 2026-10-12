#include "ce_common/as_compiler/bytecode.hpp"

#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"

namespace CE::Scripting::Impl {
    namespace Fmt = ByteCodeFormat;
    using CECommon::FS::BinaryReader;
    using CECommon::FS::BinaryWriter;
    using Fmt::ByteCodeError;
    using Fmt::ToCode;

    namespace {
        constexpr uint64_t kMaxU32 = std::numeric_limits<uint32_t>::max();

        // Reads `length` bytes, refusing to read past `end`.
        bool ReadBoundedString(BinaryReader& reader, uint64_t length, uint64_t end, std::string& out) {
            const uint64_t position = reader.Tell();
            if (position > end || length > end - position) {
                return false;
            }
            out.clear();
            if (length == 0) {
                return true;
            }
            return reader.ReadString(out, static_cast<size_t>(length));
        }

        // Reads and validates [uint32 count][uint64 end_offset].
        ByteCodeError ReadTableMeta(BinaryReader& reader, uint64_t file_size, uint64_t min_record_size,
                                    uint32_t& count, uint64_t& end_offset) {
            if (!reader.ReadU32(count) || !reader.ReadU64(end_offset)) {
                return ByteCodeError::Io;
            }
            if (count == 0) {
                return end_offset == 0 ? ByteCodeError::None : ByteCodeError::Corrupt;
            }

            const uint64_t position = reader.Tell();
            if (end_offset > file_size || end_offset < position) {
                return ByteCodeError::Corrupt;
            }
            // Guards against absurd counts causing huge allocations.
            if (static_cast<uint64_t>(count) > (end_offset - position) / min_record_size) {
                return ByteCodeError::Corrupt;
            }
            return ByteCodeError::None;
        }

        bool FitsU32(std::string_view value) {
            return value.size() <= kMaxU32;
        }

        bool WriteSizedLength(BinaryWriter& writer, std::string_view value) {
            return writer.WriteU32(static_cast<uint32_t>(value.size()));
        }
    }
    
    int ByteCodeReader::Load() {
        if (mLoaded) {
            return mLoadResult;
        }
        mLoaded = true;
        mLoadResult = LoadImpl();
        if (mLoadResult != 0) {
            mDebugInfo.clear();
            mEntryPoints.clear();
            mByteCodeStream.reset();
        }
        return mLoadResult;
    }

    int ByteCodeReader::LoadImpl() {
        BinaryReader reader(mBytecode);

        if (!reader.Seek(0, std::ios::end)) {
            return ToCode(ByteCodeError::Io);
        }
        mFileSize = reader.Tell();
        if (!reader.Seek(0, std::ios::beg)) {
            return ToCode(ByteCodeError::Io);
        }
        if (mFileSize < Fmt::kHeaderSize) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (const int result = ParseHeader(reader); result != 0) return result;
        if (const int result = ParseEntryPoints(reader); result != 0) return result;
        if (const int result = ParseBytecode(reader); result != 0) return result;
        if (const int result = ParseDebugInfomation(reader); result != 0) return result;
        return 0;
    }

    int ByteCodeReader::ParseHeader(BinaryReader& binary_reader) {
        std::string magic;
        if (!binary_reader.ReadString(magic, Fmt::kMagic.size())) {
            return ToCode(ByteCodeError::Io);
        }
        if (magic != Fmt::kMagic) {
            return ToCode(ByteCodeError::BadMagic);
        }

        uint8_t debug_flag = 0;
        if (!binary_reader.ReadU8(mHeader.version) ||
            !binary_reader.ReadU8(debug_flag) ||
            !binary_reader.ReadU64(mHeader.debug_offset) ||
            !binary_reader.ReadU64(mHeader.bytecode_offset) ||
            !binary_reader.ReadU64(mHeader.bytecode_size) ||
            !binary_reader.ReadU64(mHeader.entry_offset)) {
            return ToCode(ByteCodeError::Io);
        }

        if (mHeader.version != Fmt::kVersion) {
            return ToCode(ByteCodeError::UnsupportedVersion);
        }
        if (debug_flag != Fmt::kTrue && debug_flag != Fmt::kFalse) {
            return ToCode(ByteCodeError::Corrupt);
        }
        mHeader.has_debug = debug_flag == Fmt::kTrue;

        // Debug table location MUST be 0 when there is no debug info.
        if (mHeader.has_debug) {
            if (mHeader.debug_offset < Fmt::kHeaderSize ||
                mHeader.debug_offset > mFileSize - Fmt::kTableMetaSize) {
                return ToCode(ByteCodeError::Corrupt);
            }
        } else if (mHeader.debug_offset != 0) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (mHeader.entry_offset < Fmt::kHeaderSize ||
            mHeader.entry_offset > mFileSize - Fmt::kTableMetaSize) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (mHeader.bytecode_offset < Fmt::kHeaderSize ||
            mHeader.bytecode_offset > mFileSize ||
            mHeader.bytecode_size > mFileSize - mHeader.bytecode_offset) {
            return ToCode(ByteCodeError::Corrupt);
        }

        return 0;
    }

    int ByteCodeReader::ParseEntryPoints(BinaryReader& binary_reader) {
        if (!binary_reader.Seek(mHeader.entry_offset, std::ios::beg)) {
            return ToCode(ByteCodeError::Io);
        }

        uint32_t count = 0;
        uint64_t end_offset = 0;
        if (const auto error = ReadTableMeta(binary_reader, mFileSize, Fmt::kEntryPointRecordSize, count, end_offset);
            error != ByteCodeError::None) {
            return ToCode(error);
        }

        mEntryPoints.clear();
        mEntryPoints.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t mangled_length = 0;
            uint32_t identifier_length = 0;
            if (!binary_reader.ReadU32(mangled_length) || !binary_reader.ReadU32(identifier_length)) {
                return ToCode(ByteCodeError::Io);
            }

            std::string mangled;
            std::string identifier;
            if (!ReadBoundedString(binary_reader, mangled_length, end_offset, mangled) ||
                !ReadBoundedString(binary_reader, identifier_length, end_offset, identifier)) {
                return ToCode(ByteCodeError::Corrupt);
            }
            mEntryPoints.emplace_back(std::move(identifier), std::move(mangled));
        }

        if (count != 0 && binary_reader.Tell() != end_offset) {
            return ToCode(ByteCodeError::Corrupt);
        }
        return 0;
    }

    int ByteCodeReader::ParseBytecode(BinaryReader& binary_reader) {
        if (!binary_reader.Seek(mHeader.bytecode_offset, std::ios::beg)) {
            return ToCode(ByteCodeError::Io);
        }

        std::string buffer;
        if (mHeader.bytecode_size != 0 &&
            !binary_reader.ReadString(buffer, static_cast<size_t>(mHeader.bytecode_size))) {
            return ToCode(ByteCodeError::Io);
        }

        mBytecodeBuffer.clear();
        mBytecodeBuffer.str(std::move(buffer));
        mByteCodeStream = std::make_unique<ASByteCodeStream>(mBytecodeBuffer, nullptr);
        return 0;
    }

    int ByteCodeReader::ParseDebugInfomation(BinaryReader& binary_reader) {
        mDebugInfo.clear();
        if (!mHeader.has_debug) {
            return 0;
        }

        if (!binary_reader.Seek(mHeader.debug_offset, std::ios::beg)) {
            return ToCode(ByteCodeError::Io);
        }

        uint32_t count = 0;
        uint64_t end_offset = 0;
        if (const auto error = ReadTableMeta(binary_reader, mFileSize, Fmt::kSymbolRecordSize, count, end_offset);
            error != ByteCodeError::None) {
            return ToCode(error);
        }

        mDebugInfo.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t internal_length = 0;
            uint32_t qualified_length = 0;
            uint32_t display_length = 0;
            uint32_t kind_length = 0;
            if (!binary_reader.ReadU32(internal_length) ||
                !binary_reader.ReadU32(qualified_length) ||
                !binary_reader.ReadU32(display_length) ||
                !binary_reader.ReadU32(kind_length)) {
                return ToCode(ByteCodeError::Io);
            }

            GeneratedSymbolInfo symbol;
            if (!ReadBoundedString(binary_reader, internal_length, end_offset, symbol.InternalName) ||
                !ReadBoundedString(binary_reader, qualified_length, end_offset, symbol.QualifiedName) ||
                !ReadBoundedString(binary_reader, display_length, end_offset, symbol.DisplayName) ||
                !ReadBoundedString(binary_reader, kind_length, end_offset, symbol.Kind)) {
                return ToCode(ByteCodeError::Corrupt);
            }
            mDebugInfo.push_back(std::move(symbol));
        }

        if (count != 0 && binary_reader.Tell() != end_offset) {
            return ToCode(ByteCodeError::Corrupt);
        }
        return 0;
    }

    ASByteCodeStream* ByteCodeReader::GetByteCode() {
        if (Load() != 0) {
            return nullptr;
        }
        // Rewind so the bytecode can be loaded more than once.
        mBytecodeBuffer.clear();
        mBytecodeBuffer.seekg(0, std::ios::beg);
        return mByteCodeStream.get();
    }

    const std::vector<GeneratedSymbolInfo>& ByteCodeReader::GetDebugSymbols() {
        Load();
        return mDebugInfo;
    }

    std::string ByteCodeReader::GetEntryPoint(const std::string& friendly_name) {
        if (Load() != 0) {
            return {};
        }
        for (const auto& [identifier, mangled] : mEntryPoints) {
            if (identifier == friendly_name) {
                return mangled;
            }
        }
        return {};
    }

    bool ByteCodeWriter::AddEntryPoint(std::string friendly_name, std::string mangled_name) {
        if (friendly_name.empty() || mangled_name.empty()) {
            return false;
        }
        if (!FitsU32(friendly_name) || !FitsU32(mangled_name)) {
            return false;
        }
        for (const auto& entry : mEntryPoints) {
            if (entry.first == friendly_name) {
                return false;
            }
        }
        mEntryPoints.emplace_back(std::move(friendly_name), std::move(mangled_name));
        return true;
    }

    void ByteCodeWriter::SetByteCode(std::string_view bytes) {
        mBytecodeBuffer.clear();
        mBytecodeBuffer.str(std::string(bytes));
    }

    bool ByteCodeWriter::Write() {
        const std::string bytecode = mBytecodeBuffer.str();
        const bool has_debug = !mDebugInfo.empty();

        if (mEntryPoints.size() > kMaxU32 || mDebugInfo.size() > kMaxU32) {
            return false;
        }
        for (const auto& symbol : mDebugInfo) {
            if (!FitsU32(symbol.InternalName) || !FitsU32(symbol.QualifiedName) ||
                !FitsU32(symbol.DisplayName) || !FitsU32(symbol.Kind)) {
                return false;
            }
        }

        uint64_t entry_table_size = Fmt::kTableMetaSize;
        for (const auto& [identifier, mangled] : mEntryPoints) {
            entry_table_size += Fmt::kEntryPointRecordSize + identifier.size() + mangled.size();
        }

        uint64_t debug_table_size = 0;
        if (has_debug) {
            debug_table_size = Fmt::kTableMetaSize;
            for (const auto& symbol : mDebugInfo) {
                debug_table_size += Fmt::kSymbolRecordSize + symbol.InternalName.size() +
                                    symbol.QualifiedName.size() + symbol.DisplayName.size() +
                                    symbol.Kind.size();
            }
        }

        const uint64_t entry_offset = Fmt::kHeaderSize;
        const uint64_t entry_end = entry_offset + entry_table_size;
        const uint64_t bytecode_offset = entry_end;
        const uint64_t bytecode_size = bytecode.size();
        // Debug table location MUST be 0 when there is no debug info.
        const uint64_t debug_offset = has_debug ? bytecode_offset + bytecode_size : 0;
        const uint64_t debug_end = debug_offset + debug_table_size;

        BinaryWriter writer(mOutput);

        // header
        if (!writer.WriteString(Fmt::kMagic) ||
            !writer.WriteU8(Fmt::kVersion) ||
            !writer.WriteU8(has_debug ? Fmt::kTrue : Fmt::kFalse) ||
            !writer.WriteU64(debug_offset) ||
            !writer.WriteU64(bytecode_offset) ||
            !writer.WriteU64(bytecode_size) ||
            !writer.WriteU64(entry_offset)) {
            return false;
        }

        // When the count is 0, the end offset MUST be 0.
        if (!writer.WriteU32(static_cast<uint32_t>(mEntryPoints.size())) ||
            !writer.WriteU64(mEntryPoints.empty() ? 0 : entry_end)) {
            return false;
        }
        for (const auto& [identifier, mangled] : mEntryPoints) {
            if (!WriteSizedLength(writer, mangled) ||
                !WriteSizedLength(writer, identifier) ||
                !writer.WriteString(mangled) ||
                !writer.WriteString(identifier)) {
                return false;
            }
        }

        if (!writer.WriteString(bytecode)) {
            return false;
        }

        // debug symbols
        if (has_debug) {
            if (!writer.WriteU32(static_cast<uint32_t>(mDebugInfo.size())) ||
                !writer.WriteU64(debug_end)) {
                return false;
            }
            for (const auto& symbol : mDebugInfo) {
                if (!WriteSizedLength(writer, symbol.InternalName) ||
                    !WriteSizedLength(writer, symbol.QualifiedName) ||
                    !WriteSizedLength(writer, symbol.DisplayName) ||
                    !WriteSizedLength(writer, symbol.Kind) ||
                    !writer.WriteString(symbol.InternalName) ||
                    !writer.WriteString(symbol.QualifiedName) ||
                    !writer.WriteString(symbol.DisplayName) ||
                    !writer.WriteString(symbol.Kind)) {
                    return false;
                }
            }
        }

        mOutput.flush();
        return mOutput.good();
    }
}