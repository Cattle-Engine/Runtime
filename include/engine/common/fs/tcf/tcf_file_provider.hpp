#include <cstdint>
#include <filesystem>
#include <memory>

#include "engine/common/fs/file_provider.hpp"
#include "engine/common/fs/tcf/tcf.hpp"

namespace CE::Common::FS::TCF {
    class VfsTcfFile final : public VFS::IFile {
      public:
        VfsTcfFile(TCFArchive& archive, const std::string& path);

        bool IsOpen() const override;
        uint64_t Size() const override;
        uint64_t TellR() override;
        bool SeekR(int64_t offset, VFS::SeekOrigin origin) override;
        bool Read(void* buffer, size_t bytes) override;
        bool Write(const void* buffer, size_t bytes) override;
        bool Flush() override;

        virtual bool SeekW(int64_t offset, VFS::SeekOrigin origin) override;
        virtual uint64_t TellW() override;

        bool Eof() const override;

      private:
        TCFFile mFile;
    };

    class TCFFileProvider final : public VFS::IFileProvider {
      public:
        TCFFileProvider(const std::filesystem::path& path_to_archive);
        std::unique_ptr<VFS::IFile> OpenFile(std::string_view relative_path, VFS::OpenFlags flags) override;
        bool GetFileSize(std::string_view relative_path, uint64_t& size) const override;
        bool FileExists(std::string_view relative_path) const override;
        bool CreateFile(std::string_view relative_path) override;
        bool IsReadOnly() const override;
        std::string GetProviderName() const override;

      private:
        TCFArchive mArchive;
    };
} // namespace CE::Common::FS::TCF