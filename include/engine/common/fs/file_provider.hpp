#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace CE::Common::FS::VFS {
    enum class SeekOrigin {
        Begin,
        Current,
        End
    };

    enum class OpenFlags : unsigned {
        None = 0,
        Read = 1u << 0,
        Write = 1u << 1,
        Create = 1u << 2,
        Append = 1u << 3
    };

    constexpr OpenFlags operator|(OpenFlags lhs, OpenFlags rhs) {
        return static_cast<OpenFlags>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
    }

    constexpr OpenFlags operator&(OpenFlags lhs, OpenFlags rhs) {
        return static_cast<OpenFlags>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
    }

    constexpr bool HasFlag(OpenFlags value, OpenFlags flag) {
        return (value & flag) != OpenFlags::None;
    }

    struct DirectoryContent {
        enum class Type {
            Directory,
            File
        };

        std::string name;
        Type type;
    };

    class IFile;

    class IFileProvider {
      public:
        virtual ~IFileProvider() = default;

        // File providers that are read only will return false for anything that requires writing.
        // Eg MoveFile or DeleteDir
        virtual std::unique_ptr<IFile> OpenFile(std::string_view relative_path, OpenFlags flags) = 0;
        virtual bool GetFileSize(std::string_view relative_path, uint64_t& size) const = 0;
        virtual bool FileExists(std::string_view relative_path) const = 0;
        virtual bool CreateFile(std::string_view relative_path) = 0;
        virtual bool DeleteFile(std::string_view relative_path) = 0;

        virtual bool DirExists(std::string_view relative_path) const = 0;
        virtual std::vector<DirectoryContent> ListDirectory(std::string_view relative_path) const = 0;
        virtual bool CreateDir(std::string_view relative_path) = 0;
        virtual bool DeleteDir(std::string_view relative_path) = 0;

        virtual bool MoveFile(std::string_view old_path, std::string_view new_path) = 0;
        virtual bool MoveDir(std::string_view old_path, std::string_view new_path) = 0;

        // Most return a unix timestamp in miliseconds since the unix epoch
        virtual bool GetDirModifiedTimestamp(std::string_view path, int64_t& timestamp) = 0;

        virtual bool IsReadOnly() const = 0;
        virtual std::string GetProviderName() const = 0;
    };

    class IFile {
      public:
        virtual ~IFile() = default;

        virtual bool IsOpen() const = 0;
        virtual uint64_t Size() const = 0;
        virtual uint64_t TellR() = 0;
        virtual bool SeekR(int64_t offset, SeekOrigin origin) = 0;
        virtual bool Read(void* buffer, size_t bytes) = 0;
        virtual bool Write(const void* buffer, size_t bytes) = 0;
        virtual bool SeekW(int64_t offset, SeekOrigin origin) = 0;
        // Most return a unix timestamp in miliseconds since the unix epoch
        virtual int64_t GetDateModified() = 0;
        virtual uint64_t TellW() = 0;
        virtual bool Flush() = 0;
        virtual bool Eof() const = 0;
    };
} // namespace CE::Common::FS::VFS