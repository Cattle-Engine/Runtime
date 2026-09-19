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

    enum class OpenMode {
        Read,
        Write,
        ReadWrite,
        Create,
        CreateTruncate,
        Append
    };

    class IFile;

    class IFileProvider {
        public:
            ~IFileProvider() = default;

            virtual std::unique_ptr<IFile> OpenFile(std::string_view relative_path, OpenMode mode) = 0;
            virtual bool GetFileSize(std::string_view relative_path, size_t& out) = 0;
            virtual bool FileExists(std::string_view relative_path) = 0;
            virtual bool CreateFile(std::string_view relative_path) = 0;
            virtual bool IsReadOnly() = 0;

            virtual std::string GetProviderName() = 0;
    };

    class IFile {
        public:
            virtual ~IFile() = default;

            virtual bool IsOpen() = 0;
            virtual uint64_t Size() const = 0;
            virtual uint64_t Tell() const = 0;

            virtual bool Seek(int64_t offset, SeekOrigin origin) = 0;

            virtual bool Read(void* buffer, size_t bytes) = 0;
            virtual bool Write(const void* buffer, size_t bytes) = 0;

            virtual bool Flush() = 0;

            virtual bool Eof() const = 0;
            virtual bool HasError() const = 0;
            virtual void ClearError() = 0;    
        protected:
            std::shared_ptr<IFileProvider> pFileProtector;
    };
}