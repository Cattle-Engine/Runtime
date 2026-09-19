#pragma once

#include <memory>
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
                    .mount_point = v_mount_path,
                    .file_provider = std::make_unique<T>(std::forward<Args>(args)...)
                    .priority = priority
                };

                mMountPoints.push_back(std::move(mount));
            }

            std::string GetProviderNameOfMountPath(std::string_view v_path);
            void Unmount(std::string_view v_mount_path);
            bool FileExists(std::string_view path);
        private:
            std::vector<MountPoint> mMountPoints;
    };
}