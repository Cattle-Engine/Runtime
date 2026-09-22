#include "engine/common/fs/os_fs/directory_file_provider.hpp"

#include <complex>
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

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
        return fs::is_regular_file(file);
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

    int64_t DirectoryFile::GetDateModified() {
        const auto ftime = std::filesystem::last_write_time(mPath);
        const auto system_time = decltype(ftime)::clock::to_sys(ftime);

        return std::chrono::duration_cast<std::chrono::milliseconds>(
            system_time.time_since_epoch()
        ).count();
    }

    bool DirectoryFileProvider::DeleteFile(std::string_view relative_path) {
        fs::path path = mBasePath / relative_path;
        
        if (!fs::is_regular_file(path)) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] {} is not a directory", path.string());
            return true;
        }

        std::error_code ec;
        fs::remove(path, ec);

        if (ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to delete file: {}", ec.message());
            return false;
        }
        return true;
    }

    bool DirectoryFileProvider::DirExists(std::string_view relative_path) const {
        fs::path path = mBasePath / relative_path;
        return fs::is_directory(path);
    }

    bool DirectoryFileProvider::CreateDir(std::string_view relative_path) {
        std::error_code ec;
        fs::create_directory(mBasePath / relative_path, ec);

        if (ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to create directory: {}", ec.message());
            return false;
        }

        return true;
    }

    bool DirectoryFileProvider::DeleteDir(std::string_view relative_path) {
        fs::path path = mBasePath / relative_path;
        
        if (!fs::is_directory(path)) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] {} is not a directory", path.string());
            return true;
        }

        std::error_code ec;
        fs::remove(path, ec);

        if (ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to delete file: {}", ec.message());
            return false;
        }
        return true;
    }

    bool DirectoryFileProvider::MoveFile(std::string_view old_path, std::string_view new_path) {
        fs::path oldn = mBasePath / old_path;

        if (!fs::is_regular_file(oldn)) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] {} is not a file", oldn.string());
            return false;
        }

        std::error_code ec;
        fs::path newn = mBasePath / new_path;

        fs::rename(oldn, newn, ec);

        if (!ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to move {} to {}, {}", oldn.string(), newn.string(), ec.message());
            return false;
        }
        return true;
    }

    bool DirectoryFileProvider::MoveDir(std::string_view old_path, std::string_view new_path) {
        fs::path oldn = mBasePath / old_path;

        if (!fs::is_directory(oldn)) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] {} is not a file", oldn.string());
            return false;
        }

        std::error_code ec;
        fs::path newn = mBasePath / new_path;

        fs::rename(oldn, newn, ec);

        if (!ec) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] Failed to move {} to {}, {}", oldn.string(), newn.string(), ec.message());
            return false;
        }
        return true;
    }

    bool DirectoryFileProvider::GetDirModifiedTimestamp(std::string_view path, int64_t& timestamp) {
        fs::path full_path = mBasePath / path;
        
        if (!fs::is_directory(full_path)) {
            CE_LOG(LogLevel::Error, "[DirectoryFileProvider] {} is not a directory", full_path.string());
            return false;
        }

        const auto ftime = std::filesystem::last_write_time(full_path);
        const auto system_time = decltype(ftime)::clock::to_sys(ftime);

        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            system_time.time_since_epoch()
        ).count();
        return true;
    }

    std::vector<VFS::DirectoryContent> DirectoryFileProvider::ListDirectory(std::string_view relative_path) const {
        std::vector<VFS::DirectoryContent> contents;
        for (const auto& content : fs::directory_iterator(mBasePath / relative_path)) {
            if (content.is_regular_file()) {
                contents.push_back({content.path().filename().string(), VFS::DirectoryContent::Type::File});
            } else if (content.is_directory()) {
                contents.push_back({content.path().filename().string(), VFS::DirectoryContent::Type::Directory});
            }
        }

        return contents;
    }
} // namespace CE::Common::FS::OSFS