#include "ce_common/as_compiler/bytecode.hpp"

#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
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

        bool FitsOptionalU32(const std::optional<std::string>& value) {
            return !value.has_value() || value->size() <= kMaxU32;
        }

        bool WriteSizedLength(BinaryWriter& writer, std::string_view value) {
            return writer.WriteU32(static_cast<uint32_t>(value.size()));
        }

        // Length of an optional string as stored on disk (absent == 0).
        uint32_t StoredLength(const std::optional<std::string>& value) {
            return value.has_value() ? static_cast<uint32_t>(value->size()) : 0;
        }

        uint8_t SectionFlags(const ScriptSectionInfo& section) {
            uint8_t flags = 0;
            if (section.Source.has_value()) {
                flags |= Fmt::kSectionHasSource;
            }
            if (section.GeneratedSource.has_value()) {
                flags |= Fmt::kSectionHasGeneratedSource;
            }
            return flags;
        }

        // Whether a table location (meta + at least nothing else) fits inside the file.
        bool TableOffsetValid(uint64_t offset, uint64_t header_size, uint64_t file_size) {
            return offset >= header_size && offset <= file_size - Fmt::kTableMetaSize;
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
            mSections.clear();
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
        // Smallest header of any supported version; ParseHeader checks the exact size.
        if (mFileSize < Fmt::kHeaderSizeV1) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (const int result = ParseHeader(reader); result != 0) return result;
        if (const int result = ParseEntryPoints(reader); result != 0) return result;
        if (const int result = ParseBytecode(reader); result != 0) return result;
        if (const int result = ParseDebugInfomation(reader); result != 0) return result;
        if (const int result = ParseSections(reader); result != 0) return result;
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

        if (mHeader.version < Fmt::kMinVersion || mHeader.version > Fmt::kVersion) {
            return ToCode(ByteCodeError::UnsupportedVersion);
        }

        // Version 2 added the script section table location to the header.
        mHeader.header_size = mHeader.version >= 2 ? Fmt::kHeaderSize : Fmt::kHeaderSizeV1;
        if (mFileSize < mHeader.header_size) {
            return ToCode(ByteCodeError::Corrupt);
        }
        if (mHeader.version >= 2) {
            if (!binary_reader.ReadU64(mHeader.sections_offset)) {
                return ToCode(ByteCodeError::Io);
            }
        } else {
            mHeader.sections_offset = 0;
        }

        if (debug_flag != Fmt::kTrue && debug_flag != Fmt::kFalse) {
            return ToCode(ByteCodeError::Corrupt);
        }
        mHeader.has_debug = debug_flag == Fmt::kTrue;

        // Debug table location MUST be 0 when there is no debug info.
        if (mHeader.has_debug) {
            if (!TableOffsetValid(mHeader.debug_offset, mHeader.header_size, mFileSize)) {
                return ToCode(ByteCodeError::Corrupt);
            }
        } else if (mHeader.debug_offset != 0) {
            return ToCode(ByteCodeError::Corrupt);
        }

        // Script section table location MUST be 0 when there are no script sections.
        mHeader.has_sections = mHeader.sections_offset != 0;
        if (mHeader.has_sections &&
            !TableOffsetValid(mHeader.sections_offset, mHeader.header_size, mFileSize)) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (!TableOffsetValid(mHeader.entry_offset, mHeader.header_size, mFileSize)) {
            return ToCode(ByteCodeError::Corrupt);
        }

        if (mHeader.bytecode_offset < mHeader.header_size ||
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

    int ByteCodeReader::ParseSections(BinaryReader& binary_reader) {
        mSections.clear();
        if (!mHeader.has_sections) {
            return 0;
        }

        if (!binary_reader.Seek(mHeader.sections_offset, std::ios::beg)) {
            return ToCode(ByteCodeError::Io);
        }

        uint32_t count = 0;
        uint64_t end_offset = 0;
        if (const auto error = ReadTableMeta(binary_reader, mFileSize, Fmt::kSectionRecordSize, count, end_offset);
            error != ByteCodeError::None) {
            return ToCode(error);
        }

        mSections.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t name_length = 0;
            uint32_t source_length = 0;
            uint32_t generated_length = 0;
            uint8_t flags = 0;
            if (!binary_reader.ReadU32(name_length) ||
                !binary_reader.ReadU32(source_length) ||
                !binary_reader.ReadU32(generated_length) ||
                !binary_reader.ReadU8(flags)) {
                return ToCode(ByteCodeError::Io);
            }

            if ((flags & ~Fmt::kSectionFlagMask) != 0) {
                return ToCode(ByteCodeError::Corrupt);
            }
            const bool has_source = (flags & Fmt::kSectionHasSource) != 0;
            const bool has_generated = (flags & Fmt::kSectionHasGeneratedSource) != 0;
            // A length without its flag would leave unaccounted bytes in the record.
            if ((!has_source && source_length != 0) || (!has_generated && generated_length != 0)) {
                return ToCode(ByteCodeError::Corrupt);
            }
            if (name_length == 0) {
                return ToCode(ByteCodeError::Corrupt);
            }

            ScriptSectionInfo section;
            if (!ReadBoundedString(binary_reader, name_length, end_offset, section.Name)) {
                return ToCode(ByteCodeError::Corrupt);
            }
            for (const auto& existing : mSections) {
                if (existing.Name == section.Name) {
                    return ToCode(ByteCodeError::Corrupt);
                }
            }

            if (has_source) {
                std::string text;
                if (!ReadBoundedString(binary_reader, source_length, end_offset, text)) {
                    return ToCode(ByteCodeError::Corrupt);
                }
                section.Source = std::move(text);
            }
            if (has_generated) {
                std::string text;
                if (!ReadBoundedString(binary_reader, generated_length, end_offset, text)) {
                    return ToCode(ByteCodeError::Corrupt);
                }
                section.GeneratedSource = std::move(text);
            }
            mSections.push_back(std::move(section));
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

    const std::vector<ScriptSectionInfo>& ByteCodeReader::GetSections() {
        Load();
        return mSections;
    }

    const ScriptSectionInfo* ByteCodeReader::FindSection(const std::string& name) {
        if (Load() != 0) {
            return nullptr;
        }
        for (const auto& section : mSections) {
            if (section.Name == name) {
                return &section;
            }
        }
        return nullptr;
    }

    const std::string* ByteCodeReader::GetSectionSource(const std::string& name) {
        const ScriptSectionInfo* section = FindSection(name);
        return (section != nullptr && section->Source.has_value()) ? &*section->Source : nullptr;
    }

    const std::string* ByteCodeReader::GetSectionGeneratedSource(const std::string& name) {
        const ScriptSectionInfo* section = FindSection(name);
        return (section != nullptr && section->GeneratedSource.has_value()) ? &*section->GeneratedSource : nullptr;
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

    bool ByteCodeWriter::AddSection(ScriptSectionInfo section) {
        if (section.Name.empty() || !FitsU32(section.Name) ||
            !FitsOptionalU32(section.Source) || !FitsOptionalU32(section.GeneratedSource)) {
            return false;
        }
        for (const auto& existing : mSections) {
            if (existing.Name == section.Name) {
                return false;
            }
        }
        mSections.push_back(std::move(section));
        return true;
    }

    void ByteCodeWriter::SetByteCode(std::string_view bytes) {
        mBytecodeBuffer.clear();
        mBytecodeBuffer.str(std::string(bytes));
    }

    bool ByteCodeWriter::Write() {
        const std::string bytecode = mBytecodeBuffer.str();
        const bool has_debug = !mDebugInfo.empty();
        const bool has_sections = !mSections.empty();

        if (mEntryPoints.size() > kMaxU32 || mDebugInfo.size() > kMaxU32 || mSections.size() > kMaxU32) {
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

        uint64_t sections_table_size = 0;
        if (has_sections) {
            sections_table_size = Fmt::kTableMetaSize;
            for (const auto& section : mSections) {
                // AddSection validates these, but the vector is ours so re-check cheaply.
                if (section.Name.empty() || !FitsU32(section.Name) ||
                    !FitsOptionalU32(section.Source) || !FitsOptionalU32(section.GeneratedSource)) {
                    return false;
                }
                sections_table_size += Fmt::kSectionRecordSize + section.Name.size() +
                                       StoredLength(section.Source) +
                                       StoredLength(section.GeneratedSource);
            }
        }

        const uint64_t entry_offset = Fmt::kHeaderSize;
        const uint64_t entry_end = entry_offset + entry_table_size;
        const uint64_t bytecode_offset = entry_end;
        const uint64_t bytecode_size = bytecode.size();
        // Debug table location MUST be 0 when there is no debug info.
        const uint64_t debug_offset = has_debug ? bytecode_offset + bytecode_size : 0;
        const uint64_t debug_end = debug_offset + debug_table_size;
        // Sections come after the debug symbols (or directly after the bytecode without them).
        // Section table location MUST be 0 when there are no sections.
        const uint64_t sections_offset = has_sections ? bytecode_offset + bytecode_size + debug_table_size : 0;
        const uint64_t sections_end = sections_offset + sections_table_size;

        BinaryWriter writer(mOutput);

        // header
        if (!writer.WriteString(Fmt::kMagic) ||
            !writer.WriteU8(Fmt::kVersion) ||
            !writer.WriteU8(has_debug ? Fmt::kTrue : Fmt::kFalse) ||
            !writer.WriteU64(debug_offset) ||
            !writer.WriteU64(bytecode_offset) ||
            !writer.WriteU64(bytecode_size) ||
            !writer.WriteU64(entry_offset) ||
            !writer.WriteU64(sections_offset)) {
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

        // script sections (names + optional source / generated source)
        if (has_sections) {
            if (!writer.WriteU32(static_cast<uint32_t>(mSections.size())) ||
                !writer.WriteU64(sections_end)) {
                return false;
            }
            for (const auto& section : mSections) {
                if (!writer.WriteU32(static_cast<uint32_t>(section.Name.size())) ||
                    !writer.WriteU32(StoredLength(section.Source)) ||
                    !writer.WriteU32(StoredLength(section.GeneratedSource)) ||
                    !writer.WriteU8(SectionFlags(section)) ||
                    !writer.WriteString(section.Name)) {
                    return false;
                }
                if (section.Source.has_value() && !writer.WriteString(*section.Source)) {
                    return false;
                }
                if (section.GeneratedSource.has_value() && !writer.WriteString(*section.GeneratedSource)) {
                    return false;
                }
            }
        }

        mOutput.flush();
        return mOutput.good();
    }
}