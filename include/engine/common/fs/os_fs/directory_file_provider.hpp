#pragma once

#include <filesystem>
#include <fstream>
#include <memory>

#include "engine/common/fs/file_provider.hpp"

namespace fs = std::filesystem;

namespace CE::Common::FS::OSFS {
    class DirectoryFile final : public VFS::IFile {
      public:
        DirectoryFile(const fs::path& path, VFS::OpenFlags flags);

        bool IsOpen() const override;
        uint64_t Size() const override;
        uint64_t TellR() override;
        bool SeekR(int64_t offset, VFS::SeekOrigin origin) override;
        bool Read(void* buffer, size_t bytes) override;
        bool Write(const void* buffer, size_t bytes) override;
        bool SeekW(int64_t offset, VFS::SeekOrigin origin) override;
        uint64_t TellW() override;
        bool Flush() override;
        bool Eof() const override;

      private:
        std::fstream mFile;
        fs::path mPath;
    };

    class DirectoryFileProvider final : VFS::IFileProvider {
      public:
        // Base path must be an absloute path (eg: /home/arthur_dent/.config)
        DirectoryFileProvider(const fs::path& base_path);

        std::unique_ptr<VFS::IFile> OpenFile(std::string_view relative_path, VFS::OpenFlags flags) override;
        bool GetFileSize(std::string_view relative_path, uint64_t& size) const override;
        bool FileExists(std::string_view relative_path) const override;
        bool CreateFile(std::string_view relative_path) override;
        bool IsReadOnly() const override;
        std::string GetProviderName() const override;

      private:
        fs::path mBasePath;
    };
} // namespace CE::Common::FS::OSFS