#include "ce_editor/editor.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"

#include <imgui.h>

#include <algorithm>

namespace CE::Editor::UI {
    void ProjectFSBrowser::SetRoot(std::string_view vfs_root) {
        mRoot = FSNode{};
        mRoot.vfs_path = std::string(vfs_root);
        mRoot.name = vfs_root.empty() ? "/" : std::string(vfs_root.substr(vfs_root.rfind('/') + 1));
        mRoot.is_directory = true;

        LoadDirectory(mRoot);
    }

    void ProjectFSBrowser::Refresh() {
        mRoot.is_loaded = false;
        mRoot.children.clear();

        LoadDirectory(mRoot);
    }

    void ProjectFSBrowser::LoadDirectory(FSNode& node) {
        if (node.is_loaded || !node.is_directory)
            return;

        node.children.clear();

        auto& vfs = mEditorInstance.GetVFS();
        const auto entries = vfs.ListDirectory(node.vfs_path);

        for (const auto& entry : entries) {
            FSNode child;
            // Build child VFS path: parent_path + "/" + name
            child.vfs_path = node.vfs_path;
            if (!child.vfs_path.empty() && child.vfs_path.back() != '/') {
                child.vfs_path += '/';
            }
            child.vfs_path += entry.name;
            child.name = entry.name;
            child.is_directory = (entry.type == Common::FS::VFS::DirectoryContent::Type::Directory);

            node.children.push_back(std::move(child));
        }

        std::sort(
            node.children.begin(),
            node.children.end(),
            [](const FSNode& a, const FSNode& b) {
                if (a.is_directory != b.is_directory)
                    return a.is_directory > b.is_directory;

                return a.name < b.name;
            }
        );

        node.is_loaded = true;
    }

    void ProjectFSBrowser::DrawNode(FSNode& node) {
        ImGui::PushID(node.vfs_path.c_str());

        if (node.is_directory) {
            const bool open = ImGui::TreeNodeEx(
                node.name.c_str(),
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (open) {
                LoadDirectory(node);

                for (auto& child : node.children)
                    DrawNode(child);

                ImGui::TreePop();
            }
        }
        else {
            ImGui::TreeNodeEx(
                node.name.c_str(),
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );
        }

        ImGui::PopID();
    }

    void ProjectFSBrowser::Draw() {
        if (!mWindowOpen)
            return;

        bool open = mWindowOpen;
        if (!mEditorInstance.ProjectLoaded()) {
            mInitialized = false;
            mRoot = FSNode{};
            mCurrentProjectRoot.clear();

            if (ImGui::Begin(GetWindowName().c_str(), &open))
                ImGui::TextDisabled("No project loaded");

            mWindowOpen = open;
            ImGui::End();
            return;
        }

        auto project = mEditorInstance.GetProject();
        if (!project) return;

        const std::string project_root = project->GetProjectVFSRoot();

        if (!mInitialized || mCurrentProjectRoot != project_root) {
            SetRoot(project_root);
            mCurrentProjectRoot = project_root;
            mInitialized = true;
        }

        if (ImGui::Begin(GetWindowName().c_str(), &open)) {
            if (ImGui::Button("Refresh"))
                Refresh();

            ImGui::Separator();
            
            LoadDirectory(mRoot);

            for (auto& child : mRoot.children)
                DrawNode(child);
        }

        mWindowOpen = open;
        ImGui::End();
    }
}