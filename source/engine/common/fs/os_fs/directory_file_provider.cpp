#include <filesystem>

#include "engine/common/tracelog.hpp"
#include "engine/common/fs/os_fs/directory_file_provider.hpp"

namespace CE::Common::FS::OSFS {
    DirectoryFileProvider::DirectoryFileProvider(const fs::path& path) :mBasePath(path) {}

    bool DirectoryFileProvider::GetFileSize(std::string_view relative_path, uint64_t& size) const {
        fs::path file = mBasePath / relative_path;

        if (!fs::exists(file)) {
            CE_LOG(LogLevel::Error, "[OSFSFileProvider] [GetFileSize] File not found: {}", file.string());
            return false;
        }

        return fs::file_size(file);
    }

    bool DirectoryFileProvider::FileExists(std::string_view relative_path) const {
        fs::path file = mBasePath / relative_path;
        return fs::exists(file);
    }
}