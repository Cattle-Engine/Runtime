#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "engine/accs/serialization.hpp"
#include "engine/common/fs/ifile_stream.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::ACCS {
    namespace vfs_fs = CE::Common::FS::VFS;
    inline constexpr vfs_fs::OpenFlags kWorldSaveOpenFlags = vfs_fs::OpenFlags::Write | vfs_fs::OpenFlags::Truncate;

    // Writes to "<path>.tmp" first, so a failed save (full disk, crash,
    // serialisation error) never destroys the previous save.
    inline bool SaveWorldToFile(const World& world, vfs_fs::VFS& vfs, std::string_view path) {
        const std::string target(path);
        const std::string temp = target + ".tmp";

        // Open temporary file.
        auto file = vfs.OpenFile(temp, kWorldSaveOpenFlags);
        if (!file || !file->IsOpen()) {
            CE_LOG(LogLevel::Error, "[WorldSave] Failed to open temporary file '{}'", temp);
            return false;
        }

        bool serialized = false;
        bool stream_good = false;

        {
            vfs_fs::OStream out(std::move(file));

            serialized = WorldSerializer::Save(world, out);

            if (serialized) {
                out.flush();
            }

            stream_good = out.good();
        }

        if (!serialized || !stream_good) {
            CE_LOG(LogLevel::Error,
                "[WorldSave] Failed to write temporary file '{}'. Serialization: {}, Stream good: {}",
                temp, serialized, stream_good);

            if (!vfs.DeleteFile(temp)) {
                CE_LOG(LogLevel::Error, "[WorldSave] Failed to clean up '{}'", temp);
            }

            return false;
        }

        // Replace the old save with the temporary file.
        if (!vfs.MoveFile(temp, target)) {
            CE_LOG(LogLevel::Error,
                "[WorldSave] Failed to replace '{}' with '{}'",
                target, temp);

            return false;
        }

        CE_LOG(LogLevel::Info, "[WorldSave] Successfully saved '{}'", target);
        return true;
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
