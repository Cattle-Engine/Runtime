#pragma once

#include <filesystem>

#include "ce_editor/undoredo_system.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"

namespace CE::Editor {
    namespace fs = std::filesystem;

    class Project {
        public:
            // throws std::runtime error if it fails to get the ce_project.yaml etc
            Project(fs::path proj_path);

            fs::path GetProjectPath() const;
            UndoRedoSystem& GetUndoRedoSys() const;
        private:
            static constexpr const char* kProjectManifestName = "ce_project.yaml";

            mutable UndoRedoSystem mUndoRedo;
            const fs::path mProjectPath;
            GameInfo mGameInfo;
    };
}