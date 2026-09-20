#pragma once

#include <string_view>
#include <cstdint>
#include <string>
#include <memory>

namespace CE::Common::FS::VFS {
    enum class SeekOrigin {
        Begin,
        Current,
        End
    };

    enum class OpenFlags : unsigned {
        None   = 0,
        Read   = 1u << 0,
        Write  = 1u << 1,
        Create = 1u << 2,
        Append = 1u << 3
    };

    constexpr OpenFlags operator|(OpenFlags lhs, OpenFlags rhs) {
        return static_cast<OpenFlags>(
            static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)
        );
    }

    constexpr OpenFlags operator&(OpenFlags lhs, OpenFlags rhs) {
        return static_cast<OpenFlags>(
            static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)
        );
    }

    constexpr bool HasFlag(OpenFlags value, OpenFlags flag) {
        return (value & flag) != OpenFlags::None;
    }

    class IFile;

    class IFileProvider {
        public:
            ~IFileProvider() = default;

            virtual std::unique_ptr<IFile> OpenFile(std::string_view relative_path, OpenFlags flags) = 0;
            virtual bool GetFileSize(std::string_view relative_path, uint64_t& size) const = 0;
            virtual bool FileExists(std::string_view relative_path) const = 0;
            virtual bool CreateFile(std::string_view relative_path) = 0;
            virtual bool IsReadOnly() const = 0;
            virtual std::string GetProviderName() const = 0;
    };

    class IFile {
        public:
            virtual ~IFile() = default;

            virtual bool IsOpen() const = 0;
            virtual uint64_t Size() const = 0;
            virtual uint64_t Tell() const = 0;

            virtual bool Seek(int64_t offset, SeekOrigin origin) = 0;
            virtual bool Read(void* buffer, size_t bytes) = 0;

            // For IFiles in read-only mounts Write, Flush all return false
            virtual bool Write(const void* buffer, size_t bytes) = 0;
            virtual bool Flush() = 0;

            virtual bool Eof() const = 0;  
    };
}