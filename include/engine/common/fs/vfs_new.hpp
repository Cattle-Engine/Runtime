#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <cassert>
#include <string_view>
#include <vector>
#include <utility>

#include "engine/common/fs/file_provider.hpp"

namespace CE::Common::FS::VFS {
    struct MountPoint {
        std::string mount_point;
        std::shared_ptr<IFileProvider> file_provider;
        int priority = 0;
    };
    
    class VFS {
        public:
            template<typename T, typename... Args>
            void AddMountPoint(const std::string& v_mount_path, int priority, Args&&... args) {
                static_assert(std::is_base_of_v<IFileProvider, T>, "T must inherit from IFileProvider");

                MountPoint mount {
                    .mount_point = NormalisePath(v_mount_path),
                    .file_provider = std::make_shared<T>(std::forward<Args>(args)...),
                    .priority = priority
                };

                mMountPoints.push_back(std::move(mount));
            }

            std::string GetProviderNameOfMountPath(std::string v_path);
            void Unmount(std::string_view v_mount_path);
            bool FileExists(std::string_view path);
            bool GetFileSize(std::string_view path, uint64_t& out_size);
            bool IsWritable(std::string_view path);

            // creates a file with 0 bytes
            bool CreateFile(std::string_view path);

            // OpenFile resolves the path differently depending on the requested access mode.
            //
            // Read-only access:
            //   The highest-priority mounted provider containing the file is selected.
            //   Read-only providers are valid here (e.g. tcf).
            //
            // Write/create/append access:
            //   The highest-priority writable provider covering the path is selected.
            //   The file does not need to already exist, allowing creation in writable
            //   mounts even when a higher-priority read-only mount does not contain it.
            //
            // This means reads resolve to the highest-priority existing file, while
            // writes resolve to the highest-priority location capable of accepting them.
            std::unique_ptr<IFile> OpenFile(std::string_view path, OpenFlags flags = OpenFlags::Read);
        private:
            struct ResolvedPath {
                IFileProvider* provider;
                std::string relative_path;
            };

            static std::string NormalisePath(const std::string& v_path);
            static std::string ResolveVirtualPath(const std::string& path);
            ResolvedPath ResolvePath(std::string_view path);
            ResolvedPath ResolvePathForWrite(std::string_view path);            

            std::vector<MountPoint> mMountPoints;
    };
}