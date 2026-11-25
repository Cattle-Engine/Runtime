#pragma once

#include <string>

#include "ce_editor/undoredo_system.hpp"
#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"

namespace CE::Editor {
    class Project {
        public:
            // VFS virtual path of the project root mount (e.g. "/project").
            // Throws std::runtime_error if the manifest cannot be read/parsed.
            Project(Common::FS::VFS::VFS& vfs, std::string_view vfs_project_root);

            // Returns the VFS virtual path of the project root (e.g. "/project").
            std::string GetProjectVFSRoot() const;
            UndoRedoSystem& GetUndoRedoSys() const;
        private:
            static constexpr const char* kProjectManifestName = "ce_project.yaml";

            mutable UndoRedoSystem mUndoRedo;
            const std::string mProjectVFSRoot;
            GameInfo mGameInfo;
    };
}