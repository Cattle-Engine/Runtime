#include "engine/common/fs/os_fs/directory_file_provider.hpp"

#include <filesystem>
#include <fstream>
#include <memory>

#include "engine/common/fs/file_provider.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Common::FS::OSFS {
    std::ios::openmode ToOpenMode(VFS::OpenFlags flags) {
        std::ios::openmode mode = {};

        if ((static_cast<unsigned>(flags) & static_cast<unsigned>(VFS::OpenFlags::Read)) != 0) {
            mode |= std::ios::in;
        }

        if ((static_cast<unsigned>(flags) & static_cast<unsigned>(VFS::OpenFlags::Write)) != 0) {
            mode |= std::ios::out;
        }

        if ((static_cast<unsigned>(flags) & static_cast<unsigned>(VFS::OpenFlags::Append)) != 0) {
            mode |= std::ios::app;
        }

        if ((static_cast<unsigned>(flags) & static_cast<unsigned>(VFS::OpenFlags::Create)) != 0) {
            // std::ios doesn't have a direct "create" flag.
            // Opening with std::ios::out will create the file if it doesn't exist.
            mode |= std::ios::out;
        }

        return mode;
    }

    DirectoryFileProvider::DirectoryFileProvider(const fs::path& path) : mBasePath(path) {}

    bool DirectoryFileProvider::GetFileSize(std::string_view relative_path, uint64_t& size) const {
        const fs::path file = mBasePath / relative_path;

        std::error_code ec;
        const auto file_size = fs::file_size(file, ec);

        if (ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to get file size: {}", ec.message());
            return false;
        }

        size = file_size;
        return true;
    }

    bool DirectoryFileProvider::FileExists(std::string_view relative_path) const {
        fs::path file = mBasePath / relative_path;
        return fs::exists(file);
    }

    bool DirectoryFileProvider::CreateFile(std::string_view relative_path) {
        std::ofstream file(mBasePath / relative_path, std::ios::app);
        return file.is_open();
    }

    bool DirectoryFileProvider::IsReadOnly() const {
        return false;
    }

    std::string DirectoryFileProvider::GetProviderName() const {
        return "DirectoryFileProvider";
    }

    DirectoryFile::DirectoryFile(const fs::path& path, VFS::OpenFlags flags) {
        mPath = path;
        mFile.open(path, ToOpenMode(flags));
    }

    std::unique_ptr<VFS::IFile> DirectoryFileProvider::OpenFile(std::string_view relative_path, VFS::OpenFlags flags) {
        return std::make_unique<DirectoryFile>(mBasePath / relative_path, flags);
    }

    bool DirectoryFile::IsOpen() const {
        return mFile.is_open();
    }

    uint64_t DirectoryFile::Size() const {
        return fs::file_size(mPath);
    }

    uint64_t DirectoryFile::TellR() {
        return static_cast<uint64_t>(mFile.tellg());
    }

    bool DirectoryFile::SeekR(int64_t offset, VFS::SeekOrigin origin) {
        switch (origin) {
        case VFS::SeekOrigin::Begin:
            mFile.seekg(offset, std::ios::beg);
            return !mFile.fail();
        case VFS::SeekOrigin::Current:
            mFile.seekg(offset, std::ios::cur);
            return !mFile.fail();
        case VFS::SeekOrigin::End:
            mFile.seekg(offset, std::ios::end);
            return !mFile.fail();
        }

        return false;
    }

    bool DirectoryFile::Read(void* buffer, size_t bytes) {
        mFile.read(static_cast<char*>(buffer), bytes);
        return !mFile.fail();
    }

    bool DirectoryFile::Write(const void* buffer, size_t bytes) {
        mFile.write(static_cast<const char*>(buffer), bytes);
        return !mFile.fail();
    }

    bool DirectoryFile::SeekW(int64_t offset, VFS::SeekOrigin origin) {
        switch (origin) {
        case VFS::SeekOrigin::Begin:
            mFile.seekp(offset, std::ios::beg);
            return !mFile.fail();
        case VFS::SeekOrigin::Current:
            mFile.seekp(offset, std::ios::cur);
            return !mFile.fail();
        case VFS::SeekOrigin::End:
            mFile.seekp(offset, std::ios::end);
            return !mFile.fail();
        }
    }

    uint64_t DirectoryFile::TellW() {
        return static_cast<uint64_t>(mFile.tellp());
    }

    bool DirectoryFile::Flush() {
        mFile.flush();
        return !mFile.fail();
    }

    bool DirectoryFile::Eof() const {
        return mFile.eof();
    }
} // namespace CE::Common::FS::OSFS