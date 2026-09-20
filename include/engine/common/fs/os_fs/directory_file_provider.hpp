#pragma once

#include <filesystem>

#include "engine/common/fs/file_provider.hpp"

namespace fs = std::filesystem;

namespace CE::Common::FS::OSFS {
    class DirectoryFileProvider final: VFS::IFileProvider {
        public:
            // Base path must be an absloute path (eg: /home/arthur_dent/.config)
            DirectoryFileProvider(const fs::path& base_path);

            bool GetFileSize(std::string_view relative_path, uint64_t& size) const override;
            bool FileExists(std::string_view relative_path) const override;
        private:
            fs::path mBasePath;
    };
}