#pragma once

#include <angelscript.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
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
        constexpr uint8_t kVersion = 1;
        constexpr uint8_t kTrue = 1;
        constexpr uint8_t kFalse = 0;

        constexpr uint64_t kHeaderSize = 0x27;
        constexpr uint64_t kTableMetaSize = 12;
        constexpr uint64_t kEntryPointRecordSize = 8;
        constexpr uint64_t kSymbolRecordSize = 16;

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

    class ByteCodeReader {
        public:
            explicit ByteCodeReader(std::istream& stream) : mBytecode(stream) {}

            int Load();
            ASByteCodeStream* GetByteCode();
            const std::vector<GeneratedSymbolInfo>& GetDebugSymbols();
            std::string GetEntryPoint(const std::string& friendly_name);

        private:
            struct Header {
                uint8_t version = 0;
                bool has_debug = false;
                uint64_t debug_offset = 0;
                uint64_t bytecode_offset = 0;
                uint64_t bytecode_size = 0;
                uint64_t entry_offset = 0;
            };

        /*
            An CEBYC file is structured like this

            [Header][EntryPoints][AngelScript bytecode][optional debugging symbols]

            entry points are generated function names that point to engine stuff like "main" or "update"

            The header is structured like this:
            Offset  Size
            0x0     5   Magic "CEBYC"
            0x5     1   Version (unsigned 8-bit integer)
            0x6     1   Debug symbols available (unsigned 8-bit integer, 0 = false, 1 = true)
            0x7     8   Debug symbol table location (absolute file offset, unsigned 64-bit integer. This MUST be 0 when there is no debug information)
            0xF     8   AngelScript bytecode offset (unsigned 64-bit integer)
            0x17    8   AngelScript bytecode size (unsigned 64-bit integer)
            0x1F    8   Entry point table offset (absolute file offset, unsigned 64-bit integer)

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

            All offsets are absolute file offsets, measured from the beginning of the file.
            All integer fields are unsigned and stored in little-endian byte order.
        */

            int LoadImpl();
            int ParseHeader(CECommon::FS::BinaryReader& binary_reader);
            int ParseEntryPoints(CECommon::FS::BinaryReader& binary_reader);
            int ParseBytecode(CECommon::FS::BinaryReader& binary_reader);
            int ParseDebugInfomation(CECommon::FS::BinaryReader& binary_reader);

            std::istream& mBytecode;
            Header mHeader;
            uint64_t mFileSize = 0;
            bool mLoaded = false;
            int mLoadResult = 0;

            // empty when no debug info is provided
            std::vector<GeneratedSymbolInfo> mDebugInfo;
            std::vector<std::pair<std::string, std::string>> mEntryPoints;

            std::istringstream mBytecodeBuffer;
            std::unique_ptr<ASByteCodeStream> mByteCodeStream;
    };

    class ByteCodeWriter {
        public:
            explicit ByteCodeWriter(std::ostream& stream) : mOutput(stream) {}

            bool AddEntryPoint(std::string friendly_name, std::string mangled_name);

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

            std::stringstream mBytecodeBuffer;
            ASByteCodeStream mBytecodeStream{mBytecodeBuffer, &mBytecodeBuffer};
    };
}