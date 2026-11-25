#pragma once

#include "ce_editor/ui/i_window.hpp"
#include "ce_runtime/common/fs/vfs.hpp"

#include <string>
#include <vector>

namespace CE::Editor::UI {
    class ProjectFSBrowser : public IWindow {
    public:
        using IWindow::IWindow;

        std::string GetWindowName() override {
            return "Project Browser";
        }

        void Draw() override;

    private:
        struct FSNode {
            std::string vfs_path; // VFS virtual path (e.g. "/project/textures")
            std::string name;     // display name (basename)
            bool is_directory = false;
            bool is_loaded = false;

            std::vector<FSNode> children;
        };

        FSNode mRoot;
        bool mInitialized = false;
        std::string mCurrentProjectRoot; // tracks which project root we're showing

        bool mCreateDirectoryPopup = false;
        std::string mNewDirectoryName;
        std::string mCreateDirectoryParent;

        void SetRoot(std::string_view vfs_root);
        void Refresh();

        void LoadDirectory(FSNode& node);
        void DrawNode(FSNode& node);
    };
}