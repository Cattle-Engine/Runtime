#include "engine/common/fs/tcf/tcf_file_provider.hpp"

#include <memory>
#include <string>

#include "engine/common/fs/file_provider.hpp"

namespace CE::Common::FS::TCF {
    TCFFileProvider::TCFFileProvider(const std::filesystem::path& tcf_path) : mArchive(tcf_path) {}

    bool TCFFileProvider::GetFileSize(const std::string_view path, uint64_t& size) const {
        return mArchive.GetFileSize(std::string(path), size);
    }

    bool TCFFileProvider::FileExists(std::string_view relative_path) const {
        return mArchive.FileExists(std::string(relative_path));
    }

    bool TCFFileProvider::CreateFile([[maybe_unused]] std::string_view relative_path) {
        return false;
    }

    bool TCFFileProvider::IsReadOnly() const {
        return true;
    }

    std::string TCFFileProvider::GetProviderName() const {
        return "TCF_Archive";
    }

    std::unique_ptr<VFS::IFile> TCFFileProvider::OpenFile(std::string_view relative_path,
                                                          [[maybe_unused]] VFS::OpenFlags flags) {
        return std::make_unique<VfsTcfFile>(mArchive, std::string(relative_path));
    }

    VfsTcfFile::VfsTcfFile(TCFArchive& archive, const std::string& path) : mFile(archive.OpenFile(path)) {}

    bool VfsTcfFile::Flush() {
        return false;
    }

    bool VfsTcfFile::Write([[maybe_unused]] const void* buffer, [[maybe_unused]] size_t bytes) {
        return false;
    }

    bool VfsTcfFile::Eof() const {
        return mFile.Eof();
    }

    bool VfsTcfFile::SeekR(int64_t offset, VFS::SeekOrigin origin) {
        switch (origin) {
        case VFS::SeekOrigin::Begin:
            return mFile.Seek(offset, SeekMode::Start);
            break;
        case VFS::SeekOrigin::End:
            return mFile.Seek(offset, SeekMode::End);
            break;
        case VFS::SeekOrigin::Current:
            return mFile.Seek(offset, SeekMode::Current);
            break;
        }

        return false;
    }

    bool VfsTcfFile::Read(void* buffer, size_t bytes) {
        return mFile.Read(buffer, bytes);
    }

    uint64_t VfsTcfFile::TellR() {
        return mFile.Tell();
    }

    uint64_t VfsTcfFile::Size() const {
        return mFile.Size();
    }

    bool VfsTcfFile::IsOpen() const {
        return mFile.IsValid();
    }

    bool VfsTcfFile::SeekW([[maybe_unused]] int64_t offset, [[maybe_unused]] VFS::SeekOrigin origin) {
        return false;
    }

    uint64_t VfsTcfFile::TellW() {
        return false;
    }

    bool TCFFileProvider::DirExists(const std::string_view path) const {
        return mArchive.DirExists(std::string(path));
    }
} // namespace CE::Common::FS::TCF