#include "engine/common/fs/vfs_new.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include "engine/common/fs/file_provider.hpp"
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

        if (!result.empty() && result.back() == '/')
            result.pop_back();

        if (result.empty() || result[0] != '/')
            result = "/" + result;

        CE_LOG(LogLevel::Info, "[VFS] Normalised path '{}' -> '{}'", path, result);

        return result;
    }

    std::string VFS::ResolveVirtualPath(const std::string& path) {
        CE_LOG(LogLevel::Info, "[VFS] Resolving virtual path '{}'", path);

        std::filesystem::path p(path);
        std::vector<std::string> parts;

        for (const auto& part : p) {
            if (part == "." || part.empty()) {
                CE_LOG(LogLevel::Info, "[VFS] Ignoring path component '{}'", part.string());

                continue;
            }

            if (part == "..") {
                if (!parts.empty()) {
                    CE_LOG(LogLevel::Info, "[VFS] Resolving '..', removing component '{}'", parts.back());

                    parts.pop_back();
                } else {
                    CE_LOG(LogLevel::Warn, "[VFS] Ignoring '..' at virtual root");
                }

                continue;
            }

            parts.push_back(part.string());
        }

        std::string result = "/";

        for (size_t i = 0; i < parts.size(); i++) {
            result += parts[i];

            if (i + 1 < parts.size())
                result += "/";
        }

        CE_LOG(LogLevel::Info, "[VFS] Resolved virtual path '{}' -> '{}'", path, result);

        return result;
    }

    VFS::ResolvedPath VFS::ResolvePath(std::string_view path) {
        CE_LOG(LogLevel::Info, "[VFS] Resolving existing file '{}'", path);

        const std::string normalised_path = ResolveVirtualPath(NormalisePath(std::string(path)));

        MountPoint* best_mount = nullptr;
        std::string best_relative_path;

        for (auto& mount : mMountPoints) {
            const std::string& mount_path = mount.mount_point;

            CE_LOG(LogLevel::Info, "[VFS] Checking mount '{}' (priority {}) for '{}'", mount_path, mount.priority,
                   normalised_path);

            if (normalised_path.rfind(mount_path, 0) != 0) {
                CE_LOG(LogLevel::Info, "[VFS] Path '{}' does not match mount '{}'", normalised_path, mount_path);

                continue;
            }

            if (mount_path != "/" && normalised_path.length() != mount_path.length() &&
                normalised_path[mount_path.length()] != '/') {
                CE_LOG(LogLevel::Info, "[VFS] Mount '{}' is not a valid path boundary match", mount_path);

                continue;
            }

            std::string relative_path;

            if (mount_path == "/") {
                relative_path = normalised_path;
            } else {
                relative_path = normalised_path.substr(mount_path.length());

                if (relative_path.empty())
                    relative_path = "/";
            }

            CE_LOG(LogLevel::Info, "[VFS] Candidate provider '{}' with relative path '{}'",
                   mount.file_provider->GetProviderName(), relative_path);

            if (!mount.file_provider->FileExists(relative_path)) {
                CE_LOG(LogLevel::Info, "[VFS] File '{}' does not exist in provider '{}'", relative_path,
                       mount.file_provider->GetProviderName());

                continue;
            }

            if (!best_mount || mount_path.length() > best_mount->mount_point.length() ||
                (mount_path.length() == best_mount->mount_point.length() && mount.priority > best_mount->priority)) {
                CE_LOG(LogLevel::Info, "[VFS] Selecting provider '{}' at mount '{}'",
                       mount.file_provider->GetProviderName(), mount_path);

                best_mount = &mount;
                best_relative_path = std::move(relative_path);
            }
        }

        if (!best_mount) {
            CE_LOG(LogLevel::Warn, "[VFS] Could not resolve existing file '{}'", normalised_path);

            return {};
        }

        CE_LOG(LogLevel::Info, "[VFS] Resolved '{}' to provider '{}' with relative path '{}'", normalised_path,
               best_mount->file_provider->GetProviderName(), best_relative_path);

        return {.provider = best_mount->file_provider.get(), .relative_path = std::move(best_relative_path)};
    }

    VFS::ResolvedPath VFS::ResolvePathForWrite(std::string_view path) {
        CE_LOG(LogLevel::Debug, "[VFS] Resolving writable path '{}'", path);

        const std::string normalised_path = ResolveVirtualPath(NormalisePath(std::string(path)));

        MountPoint* best_mount = nullptr;
        std::string best_relative_path;

        for (auto& mount : mMountPoints) {
            const std::string& mount_path = mount.mount_point;

            CE_LOG(LogLevel::Debug, "[VFS] Checking mount '{}' (priority {}) for writable path '{}'", mount_path,
                   mount.priority, normalised_path);

            if (normalised_path.rfind(mount_path, 0) != 0) {
                CE_LOG(LogLevel::Debug, "[VFS] Path '{}' does not match mount '{}'", normalised_path, mount_path);

                continue;
            }

            if (mount_path != "/" && normalised_path.length() != mount_path.length() &&
                normalised_path[mount_path.length()] != '/') {
                CE_LOG(LogLevel::Debug, "[VFS] Mount '{}' is not a valid path boundary match", mount_path);

                continue;
            }

            if (mount.file_provider->IsReadOnly()) {
                CE_LOG(LogLevel::Info, "[VFS] Skipping read-only provider '{}' at mount '{}'",
                       mount.file_provider->GetProviderName(), mount_path);

                continue;
            }

            std::string relative_path;

            if (mount_path == "/") {
                relative_path = normalised_path;
            } else {
                relative_path = normalised_path.substr(mount_path.length());

                if (relative_path.empty())
                    relative_path = "/";
            }

            CE_LOG(LogLevel::Info, "[VFS] Writable candidate provider '{}' with relative path '{}'",
                   mount.file_provider->GetProviderName(), relative_path);

            if (!best_mount || mount_path.length() > best_mount->mount_point.length() ||
                (mount_path.length() == best_mount->mount_point.length() && mount.priority > best_mount->priority)) {
                CE_LOG(LogLevel::Info, "[VFS] Selecting writable provider '{}' at mount '{}'",
                       mount.file_provider->GetProviderName(), mount_path);

                best_mount = &mount;
                best_relative_path = std::move(relative_path);
            }
        }

        if (!best_mount) {
            CE_LOG(LogLevel::Warn, "[VFS] Could not resolve writable provider for '{}'", normalised_path);

            return {};
        }

        CE_LOG(LogLevel::Info, "[VFS] Resolved writable path '{}' to provider '{}' with relative path '{}'",
               normalised_path, best_mount->file_provider->GetProviderName(), best_relative_path);

        return {.provider = best_mount->file_provider.get(), .relative_path = std::move(best_relative_path)};
    }

    std::string VFS::GetProviderNameOfMountPath(std::string v_path) {
        CE_LOG(LogLevel::Info, "[VFS] Getting provider name for '{}'", v_path);

        auto resolved = ResolvePath(v_path);

        if (resolved.provider) {
            const std::string provider_name = resolved.provider->GetProviderName();

            CE_LOG(LogLevel::Info, "[VFS] Provider for '{}' is '{}'", v_path, provider_name);

            return provider_name;
        }

        CE_LOG(LogLevel::Warn, "[VFS] No provider found for '{}'", v_path);

        return "";
    }

    void VFS::Unmount(std::string_view v_mount_path) {
        CE_LOG(LogLevel::Info, "[VFS] Attempting to unmount '{}'", v_mount_path);

        for (auto it = mMountPoints.begin(); it != mMountPoints.end(); ++it) {
            if (it->mount_point == v_mount_path) {
                CE_LOG(LogLevel::Info, "[VFS] Unmounting '{}' from provider '{}'", it->mount_point,
                       it->file_provider->GetProviderName());

                mMountPoints.erase(it);

                CE_LOG(LogLevel::Info, "[VFS] Successfully unmounted '{}'", v_mount_path);

                return;
            }
        }

        CE_LOG(LogLevel::Error, "[VFS] Mount point not found: '{}'", v_mount_path);
    }

    bool VFS::FileExists(std::string_view path) {
        CE_LOG(LogLevel::Info, "[VFS] Checking whether file '{}' exists", path);

        const bool exists = ResolvePath(path).provider != nullptr;

        if (exists) {
            CE_LOG(LogLevel::Info, "[VFS] File '{}' exists", path);
        } else {
            CE_LOG(LogLevel::Info, "[VFS] File '{}' does not exist", path);
        }

        return exists;
    }

    bool VFS::GetFileSize(std::string_view path, uint64_t& out_size) {
        CE_LOG(LogLevel::Info, "[VFS] Getting file size for '{}'", path);

        const ResolvedPath resolved = ResolvePath(path);

        if (!resolved.provider) {
            CE_LOG(LogLevel::Error, "[VFS] Cannot get file size: '{}' could not be resolved", path);

            return false;
        }

        if (!resolved.provider->GetFileSize(resolved.relative_path, out_size)) {
            CE_LOG(LogLevel::Error, "[VFS] Provider '{}' failed to get file size for '{}'",
                   resolved.provider->GetProviderName(), resolved.relative_path);

            return false;
        }

        CE_LOG(LogLevel::Info, "[VFS] File '{}' has size {} bytes", path, out_size);

        return true;
    }

    std::unique_ptr<IFile> VFS::OpenFile(std::string_view path, OpenFlags flags) {
        CE_LOG(LogLevel::Info, "[VFS] Opening file '{}'", path);

        const bool wants_write = (flags & OpenFlags::Write) != OpenFlags::None ||
                                 (flags & OpenFlags::Create) != OpenFlags::None ||
                                 (flags & OpenFlags::Append) != OpenFlags::None;

        const ResolvedPath resolved = wants_write ? ResolvePathForWrite(path) : ResolvePath(path);

        if (!resolved.provider) {
            CE_LOG(LogLevel::Error, "[VFS] Cannot open file '{}': no suitable provider found", path);

            return nullptr;
        }

        if (wants_write && resolved.provider->IsReadOnly()) {
            CE_LOG(LogLevel::Error, "[VFS] Cannot open '{}' for writing: provider '{}' is read-only", path,
                   resolved.provider->GetProviderName());

            return nullptr;
        }

        CE_LOG(LogLevel::Info, "[VFS] Opening '{}' through provider '{}' with relative path '{}'", path,
               resolved.provider->GetProviderName(), resolved.relative_path);

        auto file = resolved.provider->OpenFile(resolved.relative_path, flags);

        if (!file) {
            CE_LOG(LogLevel::Error, "[VFS] Provider '{}' failed to open '{}'", resolved.provider->GetProviderName(),
                   resolved.relative_path);

            return nullptr;
        }

        CE_LOG(LogLevel::Info, "[VFS] Successfully opened '{}'", path);

        return file;
    }

    bool VFS::CreateFile(std::string_view path) {
        CE_LOG(LogLevel::Info, "[VFS] Creating file '{}'", path);

        const ResolvedPath resolved = ResolvePathForWrite(path);

        if (!resolved.provider) {
            CE_LOG(LogLevel::Error, "[VFS] Failed to create file '{}': no writable provider found", path);

            return false;
        }

        if (resolved.provider->IsReadOnly()) {
            CE_LOG(LogLevel::Error, "[VFS] Cannot create '{}': provider '{}' is read-only", path,
                   resolved.provider->GetProviderName());

            return false;
        }

        CE_LOG(LogLevel::Info, "[VFS] Creating '{}' through provider '{}' with relative path '{}'", path,
               resolved.provider->GetProviderName(), resolved.relative_path);

        if (!resolved.provider->CreateFile(resolved.relative_path)) {
            CE_LOG(LogLevel::Error, "[VFS] Provider '{}' failed to create '{}'", resolved.provider->GetProviderName(),
                   resolved.relative_path);

            return false;
        }

        CE_LOG(LogLevel::Info, "[VFS] Successfully created file '{}' using provider '{}'", path,
               resolved.provider->GetProviderName());

        return true;
    }

    bool VFS::IsWritable(std::string_view path) {
        return ResolvePathForWrite(path).provider != nullptr;
    }
} // namespace CE::Common::FS::VFS