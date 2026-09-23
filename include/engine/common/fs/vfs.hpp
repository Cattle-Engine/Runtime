#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <type_traits>

#include "engine/common/fs/file_provider.hpp"

namespace CE::Common::FS::VFS {
    struct MountPoint {
        struct MountID {
            MountID(const uint32_t id) : mount_id(id) {}

            uint32_t mount_id;

            bool operator==(const MountID& other) const {
                return mount_id == other.mount_id;
            }
        };

        std::string mount_point;
        std::shared_ptr<IFileProvider> file_provider;
        int priority = 0;
        MountID mount_id;
    };

    class VFS {
      public:
        template <typename T, typename... Args>
        MountPoint::MountID AddMountPoint(const std::string& v_mount_path, int priority, Args&&... args) {
            static_assert(std::is_base_of_v<IFileProvider, T>, "T must inherit from IFileProvider");

            MountPoint mount{.mount_point = NormalisePath(v_mount_path),
                             .file_provider = std::make_shared<T>(std::forward<Args>(args)...),
                             .priority = priority,
                             .mount_id = MountPoint::MountID(mNextMountID++)};

            mMountPoints.push_back(std::move(mount));
            return mMountPoints.back().mount_id;
        }

        std::string GetProviderNameOfMountPath(std::string v_path);
        void Unmount(MountPoint::MountID mount);
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

        // These can return false if it failed or its a read only mount
        bool DeleteFile(const std::string_view path);
        bool MoveFile(const std::string_view old_path, const std::string_view new_path);

        bool DirExists(const std::string_view path);
        std::vector<DirectoryContent> ListDirectory(const std::string_view path);
        bool IsDir(const std::string_view path);
        bool IsFile(const std::string_view path);
        bool DeleteDir(const std::string_view path);
        bool MoveDir(const std::string path);

      private:
        struct ResolvedPath {
            IFileProvider* provider;
            std::string relative_path;
        };

        static std::string NormalisePath(const std::string& v_path);
        static std::string ResolveVirtualPath(const std::string& path);
        /*
            Resolve's a path like this:
            1. Most specific mount path
            2. Highest priority
            3. Newest mount
        */
        ResolvedPath ResolvePath(std::string_view path);
        ResolvedPath ResolvePathForWrite(std::string_view path);

        uint32_t mNextMountID = 0;
        std::vector<MountPoint> mMountPoints;
    };
} // namespace CE::Common::FS::VFS