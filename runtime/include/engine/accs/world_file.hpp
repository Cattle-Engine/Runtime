#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "engine/accs/serialization.hpp"
#include "engine/common/fs/ifile_stream.hpp"
#include "engine/common/fs/vfs.hpp"

namespace CE::ACCS {
    namespace vfs_fs = CE::Common::FS::VFS;
    inline constexpr vfs_fs::OpenFlags kWorldSaveOpenFlags = vfs_fs::OpenFlags::Write | vfs_fs::OpenFlags::Truncate;

    // Writes to "<path>.tmp" first, so a failed save (full disk, crash,
    // serialisation error) never destroys the previous save.
    inline bool SaveWorldToFile(const World& world, vfs_fs::VFS& vfs, std::string_view path) {
        const std::string target(path);
        const std::string temp = target + ".tmp";

        bool written = false;
        {
            auto file = vfs.OpenFile(temp, kWorldSaveOpenFlags);
            if (!file) {
                return false;
            }

            vfs_fs::OStream out(std::move(file));
            written = WorldSerializer::Save(world, out) && out.good();
        } // stream destroyed here: flushed and closed

        if (!written) {
            vfs.DeleteFile(temp);
            return false;
        }

        if (vfs.FileExists(target) && !vfs.DeleteFile(target)) {
            vfs.DeleteFile(temp);
            return false;
        }

        return vfs.MoveFile(temp, target);
    }

    inline LoadResult LoadWorldFromFile(World& world, vfs_fs::VFS& vfs, std::string_view path) {
        auto file = vfs.OpenFile(path, vfs_fs::OpenFlags::Read);
        if (!file) {
            return LoadResult::Failed;
        }

        vfs_fs::IStream in(std::move(file));
        return WorldSerializer::Load(world, in);
    }
}