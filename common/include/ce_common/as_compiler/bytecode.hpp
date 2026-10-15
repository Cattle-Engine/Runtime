#pragma once

#include <angelscript.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ce_common/as_compiler/modules.hpp"
#include "ce_common/binary_reader.hpp"

namespace CE::Scripting::Impl {
    class ASByteCodeStream : public asIBinaryStream {
        public:
        explicit ASByteCodeStream(std::istream& stream, std::ostream* output)
        : mInput(stream), mOutput(output) {
        }

        int Read(void* ptr, asUINT size) override {
            mInput.read(static_cast<char*>(ptr), static_cast<std::streamsize>(size));
            return mInput.fail() ? asERROR : asSUCCESS;
        }

        int Write(const void* ptr, asUINT size) override {
            if (mOutput == nullptr) {
                return asSUCCESS;
            }

            mOutput->write(static_cast<const char*>(ptr), static_cast<std::streamsize>(size));
            return mOutput->fail() ? asERROR : asSUCCESS;
        }

        private:
        std::istream& mInput;
        std::ostream* mOutput = nullptr;
    };

    namespace ByteCodeFormat {
        constexpr std::string_view kMagic = "CEBYC";
        constexpr uint8_t kMinVersion = 1;  // oldest version the reader still understands
        constexpr uint8_t kVersion = 2;     // version the writer produces
        constexpr uint8_t kTrue = 1;
        constexpr uint8_t kFalse = 0;

        constexpr uint64_t kHeaderSizeV1 = 0x27;
        constexpr uint64_t kHeaderSize = 0x2F;  // version 2
        constexpr uint64_t kTableMetaSize = 12;
        constexpr uint64_t kEntryPointRecordSize = 8;
        constexpr uint64_t kSymbolRecordSize = 16;
        constexpr uint64_t kSectionRecordSize = 13;

        // Script section record flags
        constexpr uint8_t kSectionHasSource = 0x01;
        constexpr uint8_t kSectionHasGeneratedSource = 0x02;
        constexpr uint8_t kSectionFlagMask = kSectionHasSource | kSectionHasGeneratedSource;

        enum class ByteCodeError : int {
            None = 0,
            Io = 1,
            BadMagic = 2,
            UnsupportedVersion = 3,
            Corrupt = 4,
        };

        constexpr int ToCode(ByteCodeError error) {
            return static_cast<int>(error);
        }
    }

    // Debug information about one AngelScript script section.
    // Source and GeneratedSource are independently optional. An engaged but empty
    // string means "there is a source and it is empty", which is different from nullopt.
    struct ScriptSectionInfo {
        std::string Name;
        std::optional<std::string> Source;           // what the user wrote
        std::optional<std::string> GeneratedSource;  // what the compiler actually fed to AngelScript
    };

    class ByteCodeReader {
        public:
            explicit ByteCodeReader(std::istream& stream) : mBytecode(stream) {}

            int Load();
            ASByteCodeStream* GetByteCode();
            const std::vector<GeneratedSymbolInfo>& GetDebugSymbols();
            std::string GetEntryPoint(const std::string& friendly_name);

            // Script sections (optional debugging info, empty when the file has none)
            const std::vector<ScriptSectionInfo>& GetSections();
            const ScriptSectionInfo* FindSection(const std::string& name);
            // nullptr when the section does not exist or has no (generated) source stored
            const std::string* GetSectionSource(const std::string& name);
            const std::string* GetSectionGeneratedSource(const std::string& name);

        private:
            struct Header {
                uint8_t version = 0;
                uint64_t header_size = 0;
                bool has_debug = false;
                uint64_t debug_offset = 0;
                uint64_t bytecode_offset = 0;
                uint64_t bytecode_size = 0;
                uint64_t entry_offset = 0;
                bool has_sections = false;
                uint64_t sections_offset = 0;
            };

        /*
            An CEBYC file is structured like this

            [Header][EntryPoints][AngelScript bytecode][optional debugging symbols][optional script sections]

            entry points are generated function names that point to engine stuff like "main" or "update"

            The header is structured like this (version 2):
            Offset  Size
            0x0     5   Magic "CEBYC"
            0x5     1   Version (unsigned 8-bit integer)
            0x6     1   Debug symbols available (unsigned 8-bit integer, 0 = false, 1 = true)
            0x7     8   Debug symbol table location (absolute file offset, unsigned 64-bit integer. This MUST be 0 when there is no debug information)
            0xF     8   AngelScript bytecode offset (unsigned 64-bit integer)
            0x17    8   AngelScript bytecode size (unsigned 64-bit integer)
            0x1F    8   Entry point table offset (absolute file offset, unsigned 64-bit integer)
            0x27    8   Script section table location (absolute file offset, unsigned 64-bit integer. This MUST be 0 when there are no script sections)

            Version 1 files have the same header but stop after the entry point table offset
            (0x27 bytes) and never contain script sections. The reader accepts both versions,
            the writer always produces version 2.

            For the entry point table, it is structured like this:

            Table metadata:
            [uint32_entries_count][uint64_entries_end_offset]
            When entries count is 0, entries end offset MUST be 0
            Entries begin immediately after this

            Table entry:
            [uint32_mangled_length][uint32_identifier_length][mangled_name_data][identifier_data]

            The debugging symbol table is structured like this:

            Table metadata:
            [uint32_symbols_count][uint64_symbols_end_offset]
            When symbols count is 0, symbols end offset MUST be 0
            Symbols begin immediately after this

            Table entry:
            [uint32_internal_name_length]
            [uint32_qualified_name_length]
            [uint32_display_name_length]
            [uint32_kind_length]
            [internal_name_data]
            [qualified_name_data]
            [display_name_data]
            [kind_data]

            Each string is stored as a sequence of bytes with the length specified
            in its corresponding length field. String data is not null-terminated.

            internal_name_data is the generated symbol's internal name.
            qualified_name_data is the symbol's fully qualified name.
            display_name_data is the human-readable name of the symbol.
            kind_data identifies the kind of symbol, such as a function, variable, or type.

            The script section table (optional debugging information, version 2 and up)
            stores the names of the AngelScript script sections together with, optionally,
            the original source code and the "generated" source code of each section.
            Source lookup is done by section name. It is structured like this:

            Table metadata:
            [uint32_sections_count][uint64_sections_end_offset]
            When sections count is 0, sections end offset MUST be 0
            Sections begin immediately after this

            Table entry:
            [uint32_name_length]
            [uint32_source_length]
            [uint32_generated_source_length]
            [uint8_flags]
            [name_data]
            [source_data]
            [generated_source_data]

            flags is a bit field:
            Bit 0 (0x01)  The section has original source code stored
            Bit 1 (0x02)  The section has generated source code stored
            All other bits MUST be 0.

            A source length MUST be 0 when its flag bit is not set, and the corresponding data
            is then absent. When the flag bit is set a length of 0 is valid and means the
            source is stored but empty (not the same as "no source stored").

            Section names MUST NOT be empty and MUST be unique within the table. The name is
            the script section name that was handed to AngelScript (e.g. via AddScriptSection).
            Source strings use the same encoding as the strings above (raw bytes, no null
            terminator, line endings preserved as-is).

            All offsets are absolute file offsets, measured from the beginning of the file.
            All integer fields are unsigned and stored in little-endian byte order.
        */

            int LoadImpl();
            int ParseHeader(CECommon::FS::BinaryReader& binary_reader);
            int ParseEntryPoints(CECommon::FS::BinaryReader& binary_reader);
            int ParseBytecode(CECommon::FS::BinaryReader& binary_reader);
            int ParseDebugInfomation(CECommon::FS::BinaryReader& binary_reader);
            int ParseSections(CECommon::FS::BinaryReader& binary_reader);

            std::istream& mBytecode;
            Header mHeader;
            uint64_t mFileSize = 0;
            bool mLoaded = false;
            int mLoadResult = 0;

            // empty when no debug info is provided
            std::vector<GeneratedSymbolInfo> mDebugInfo;
            std::vector<ScriptSectionInfo> mSections;
            std::vector<std::pair<std::string, std::string>> mEntryPoints;

            std::istringstream mBytecodeBuffer;
            std::unique_ptr<ASByteCodeStream> mByteCodeStream;
    };

    class ByteCodeWriter {
        public:
            explicit ByteCodeWriter(std::ostream& stream) : mOutput(stream) {}

            bool AddEntryPoint(std::string friendly_name, std::string mangled_name);

            // Adds a script section (name plus optional source / generated source).
            // Returns false if the name is empty, already added, or a string is too large.
            bool AddSection(ScriptSectionInfo section);

            ASByteCodeStream& GetByteCodeSink() {
                return mBytecodeStream;
            }

            void SetByteCode(std::string_view bytes);

            void SetDebugSymbols(std::vector<GeneratedSymbolInfo> symbols) {
                mDebugInfo = std::move(symbols);
            }

            bool Write();

        private:
            std::ostream& mOutput;
            std::vector<std::pair<std::string, std::string>> mEntryPoints;
            std::vector<GeneratedSymbolInfo> mDebugInfo;
            std::vector<ScriptSectionInfo> mSections;

            std::stringstream mBytecodeBuffer;
            ASByteCodeStream mBytecodeStream{mBytecodeBuffer, &mBytecodeBuffer};
    };
}