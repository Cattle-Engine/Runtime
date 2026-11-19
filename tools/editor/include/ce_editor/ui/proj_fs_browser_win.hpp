#pragma once

#include "ce_editor/ui/i_window.hpp"

#include <filesystem>
#include <vector>

namespace CE::Editor::UI {
    namespace fs = std::filesystem;

    class ProjectFSBrowser : public IWindow {
    public:
        using IWindow::IWindow;

        std::string GetWindowName() override {
            return "Project Browser";
        }

        void Draw() override;

    private:
        struct FSNode {
            fs::path path;
            bool is_directory = false;
            bool is_loaded = false;

            std::vector<FSNode> children;
        };

        FSNode mRoot;
        bool mInitialized = false;

        void SetRoot(const fs::path& path);
        void Refresh();

        void LoadDirectory(FSNode& node);
        void DrawNode(FSNode& node);
    };
}