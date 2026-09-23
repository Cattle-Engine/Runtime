#include "engine/common/fs/vfs.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "engine/common/tracelog.hpp"

namespace CE::Common::FS::VFS {
    std::string VFS::NormalisePath(const std::string& path) {
        std::string result;
        bool last_was_slash = false;

        for (char c : path) {
            if (c == '/' || c == '\\') {
                if (!last_was_slash) {
                    result += '/';
                    last_was_slash = true;
                }
            } else {
                result += c;
                last_was_slash = false;
            }
        }

        if (!result.empty() && result.back() == '/') {
            result.pop_back();
        }

        if (result.empty() || result[0] != '/') {
            result = "/" + result;
        }

        return result;
    }

    std::string VFS::ResolveVirtualPath(const std::string& path) {
        std::filesystem::path p(path);
        std::vector<std::string> parts;

        for (const auto& part : p) {
            const std::string component = part.string();

            if (component == "/" || component == "." || component.empty()) {
                continue;
            }

            if (component == "..") {
                if (!parts.empty()) {
                    parts.pop_back();
                }

                continue;
            }

            parts.push_back(component);
        }

        std::string result = "/";

        for (size_t i = 0; i < parts.size(); ++i) {
            result += parts[i];

            if (i + 1 < parts.size()) {
                result += "/";
            }
        }

        return result;
    }

    VFS::ResolvedPath VFS::ResolvePath(std::string_view path) {
        const std::string normalised_path =
            ResolveVirtualPath(NormalisePath(std::string(path)));

        MountPoint* best_mount = nullptr;
        std::string best_relative_path;

        for (auto& mount : mMountPoints) {
            const std::string& mount_path = mount.mount_point;

            // Check prefix.
            if (normalised_path.rfind(mount_path, 0) != 0) {
                continue;
            }

            // Check path boundary.
            if (mount_path != "/" &&
                normalised_path.length() != mount_path.length() &&
                normalised_path[mount_path.length()] != '/') {
                continue;
            }

            std::string relative_path;

            if (mount_path == "/") {
                relative_path = normalised_path;
            } else {
                relative_path = normalised_path.substr(mount_path.length());

                if (relative_path.empty()) {
                    relative_path = "/";
                }
            }

            // Only existing files are candidates for reads.
            if (!mount.file_provider->FileExists(relative_path)) {
                continue;
            }

            // Most specific mount wins.
            // For equal specificity, higher priority wins.
            // For equal priority, newest mount wins.
            if (!best_mount ||
                mount_path.length() > best_mount->mount_point.length() ||
                (mount_path.length() == best_mount->mount_point.length() &&
                 mount.priority >= best_mount->priority)) {
                best_mount = &mount;
                best_relative_path = std::move(relative_path);
            }
        }

        if (!best_mount) {
            return {};
        }

        return {
            .provider = best_mount->file_provider.get(),
            .relative_path = std::move(best_relative_path)
        };
    }

    VFS::ResolvedPath VFS::ResolvePathForWrite(std::string_view path) {
        const std::string normalised_path =
            ResolveVirtualPath(NormalisePath(std::string(path)));

        MountPoint* best_mount = nullptr;
        std::string best_relative_path;

        for (auto& mount : mMountPoints) {
            const std::string& mount_path = mount.mount_point;

            if (normalised_path.rfind(mount_path, 0) != 0) {
                continue;
            }

            if (mount_path != "/" &&
                normalised_path.length() != mount_path.length() &&
                normalised_path[mount_path.length()] != '/') {
                continue;
            }

            if (mount.file_provider->IsReadOnly()) {
                continue;
            }

            std::string relative_path;

            if (mount_path == "/") {
                relative_path = normalised_path;
            } else {
                relative_path = normalised_path.substr(mount_path.length());

                if (relative_path.empty()) {
                    relative_path = "/";
                }
            }

            if (!best_mount ||
                mount_path.length() > best_mount->mount_point.length() ||
                (mount_path.length() == best_mount->mount_point.length() &&
                 mount.priority >= best_mount->priority)) {
                best_mount = &mount;
                best_relative_path = std::move(relative_path);
            }
        }

        if (!best_mount) {
            return {};
        }

        return {
            .provider = best_mount->file_provider.get(),
            .relative_path = std::move(best_relative_path)
        };
    }

    std::string VFS::GetProviderNameOfMountPath(std::string v_path) {
        const ResolvedPath resolved = ResolvePath(v_path);

        if (!resolved.provider) {
            return "";
        }

        return resolved.provider->GetProviderName();
    }

    void VFS::Unmount(MountPoint::MountID mount) {
        const auto it = std::find_if(
            mMountPoints.begin(),
            mMountPoints.end(),
            [&mount](const MountPoint& point) {
                return point.mount_id.mount_id == mount.mount_id;
            }
        );

        if (it == mMountPoints.end()) {
            CE_LOG(LogLevel::Warn,
                   "[VFS] Mount ID '{}' not found",
                   mount.mount_id);

            return;
        }

        CE_LOG(LogLevel::Info,
               "[VFS] Unmounting '{}' from provider '{}'",
               it->mount_point,
               it->file_provider->GetProviderName());

        mMountPoints.erase(it);
    }

    bool VFS::FileExists(std::string_view path) {
        return ResolvePath(path).provider != nullptr;
    }

    bool VFS::GetFileSize(std::string_view path, uint64_t& out_size) {
        const ResolvedPath resolved = ResolvePath(path);

        if (!resolved.provider) {
            return false;
        }

        return resolved.provider->GetFileSize(
            resolved.relative_path,
            out_size
        );
    }

    bool VFS::IsWritable(std::string_view path) {
        return ResolvePathForWrite(path).provider != nullptr;
    }

    bool VFS::CreateFile(std::string_view path) {
        const ResolvedPath resolved = ResolvePathForWrite(path);

        if (!resolved.provider) {
            return false;
        }

        return resolved.provider->CreateFile(resolved.relative_path);
    }

    std::unique_ptr<IFile> VFS::OpenFile(
        std::string_view path,
        OpenFlags flags
    ) {
        const bool wants_write =
            (flags & OpenFlags::Write) != OpenFlags::None ||
            (flags & OpenFlags::Create) != OpenFlags::None ||
            (flags & OpenFlags::Append) != OpenFlags::None;

        const ResolvedPath resolved = wants_write
            ? ResolvePathForWrite(path)
            : ResolvePath(path);

        if (!resolved.provider) {
            return nullptr;
        }

        if (wants_write && resolved.provider->IsReadOnly()) {
            return nullptr;
        }

        return resolved.provider->OpenFile(
            resolved.relative_path,
            flags
        );
    }
} // namespace CE::Common::FS::VFS