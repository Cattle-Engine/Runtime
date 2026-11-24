#include "ce_editor/editor.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"

#include <imgui.h>

#include <algorithm>
#include <system_error>

namespace CE::Editor::UI {
    void ProjectFSBrowser::SetRoot(const fs::path& path) {
        mRoot = FSNode{};
        mRoot.path = path;
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

        std::error_code ec;

        fs::directory_iterator it(
            node.path,
            fs::directory_options::skip_permission_denied,
            ec
        );

        if (ec) {
            node.is_loaded = true;
            return;
        }

        for (const auto& entry : it) {
            FSNode child;

            child.path = entry.path();
            child.is_directory = entry.is_directory(ec);

            if (ec)
                ec.clear();

            node.children.push_back(std::move(child));
        }

        std::sort(
            node.children.begin(),
            node.children.end(),
            [](const FSNode& a, const FSNode& b) {
                if (a.is_directory != b.is_directory)
                    return a.is_directory > b.is_directory;

                return a.path.filename().string() <
                       b.path.filename().string();
            }
        );

        node.is_loaded = true;
    }

    void ProjectFSBrowser::DrawNode(FSNode& node) {
        const std::string name = node.path.filename().string();

        ImGui::PushID(node.path.string().c_str());

        if (node.is_directory) {
            const bool open = ImGui::TreeNodeEx(
                name.c_str(),
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
                name.c_str(),
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

            if (ImGui::Begin(GetWindowName().c_str(), &open))
                ImGui::TextDisabled("No project loaded");

            mWindowOpen = open;
            ImGui::End();
            return;
        }

        auto project = mEditorInstance.GetProject();
        if (!project) return;

        const auto& project_path = project->GetProjectPath();

        if (!mInitialized || mRoot.path != project_path) {
            SetRoot(project_path);
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