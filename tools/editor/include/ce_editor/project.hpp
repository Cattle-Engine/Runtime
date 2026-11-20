#pragma once

#include <filesystem>

#include "ce_common/result.hpp"

#include "ce_editor/undoredo_system.hpp"

namespace CE::Editor {
    namespace fs = std::filesystem;

    class Project {
        public:
            Result<fs::path> GetProjectPath() const;

            UndoRedoSystem mUndoRedo;
        private:
            fs::path mProjectPath;
    };
}