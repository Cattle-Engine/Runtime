
#include "ce_common/tracelog.hpp"
#include "ce_editor/editor.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

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

            child.vfs_path = node.vfs_path;

            if (!child.vfs_path.empty() && child.vfs_path.back() != '/')
                child.vfs_path += '/';

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
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (ImGui::BeginPopupContextItem("DirectoryContextMenu")) {
                if (ImGui::MenuItem("Refresh")) {
                    mPendingOperation = PendingOperation::Refresh;
                }

                if (ImGui::MenuItem("Create Directory")) {
                    mCreateDirectoryParent = node.vfs_path;
                    mNewDirectoryName.clear();
                    mCreateDirectoryPopup = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Delete")) {
                    mPendingOperation = PendingOperation::Delete;
                    mPendingPath = node.vfs_path;
                    mPendingIsDirectory = true;
                }

                ImGui::EndPopup();
            }

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

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (!mEditorInstance.GetAssetEditorManager().Open(node.vfs_path)) {
                    CE_LOG(
                        CECommon::LogLevel::Error,
                        "[ProjectFSBrowser] Failed to open asset, {}",
                        node.vfs_path
                    );
                }
            }

            if (ImGui::BeginPopupContextItem("FileContextMenu")) {
                if (ImGui::MenuItem("Open")) {
                    if (!mEditorInstance.GetAssetEditorManager().Open(node.vfs_path)) {
                        CE_LOG(
                            CECommon::LogLevel::Error,
                            "[ProjectFSBrowser] Failed to open asset, {}",
                            node.vfs_path
                        );
                    }
                }

                if (ImGui::MenuItem("Copy Path")) {
                    ImGui::SetClipboardText(node.vfs_path.c_str());
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Delete")) {
                    mPendingOperation = PendingOperation::Delete;
                    mPendingPath = node.vfs_path;
                    mPendingIsDirectory = false;
                }

                ImGui::EndPopup();
            }
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

        if (!project) {
            if (ImGui::Begin(GetWindowName().c_str(), &open))
                ImGui::TextDisabled("No project available");

            mWindowOpen = open;
            ImGui::End();
            return;
        }

        const std::string project_root = project->GetProjectVFSRoot();

        if (!mInitialized || mCurrentProjectRoot != project_root) {
            SetRoot(project_root);
            mCurrentProjectRoot = project_root;
            mInitialized = true;
        }

        if (ImGui::Begin(GetWindowName().c_str(), &open)) {
            if (ImGui::Button("Refresh"))
                mPendingOperation = PendingOperation::Refresh;

            ImGui::Separator();

            LoadDirectory(mRoot);

            for (auto& child : mRoot.children)
                DrawNode(child);

            if (ImGui::BeginPopupContextWindow(
                "BackgroundContextMenu",
                ImGuiPopupFlags_MouseButtonRight |
                ImGuiPopupFlags_NoOpenOverItems
            )) {
                if (ImGui::MenuItem("Create Directory")) {
                    mCreateDirectoryParent = mRoot.vfs_path;
                    mNewDirectoryName.clear();
                    mCreateDirectoryPopup = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Refresh"))
                    mPendingOperation = PendingOperation::Refresh;

                ImGui::EndPopup();
            }
        }

        // Process pending filesystem operations after recursive traversal.
        switch (mPendingOperation) {
            case PendingOperation::Refresh:
                Refresh();
                break;

            case PendingOperation::Delete: {
                auto& vfs = mEditorInstance.GetVFS();

                if (mPendingIsDirectory)
                    vfs.DeleteDir(mPendingPath);
                else
                    vfs.DeleteFile(mPendingPath);

                Refresh();
                break;
            }

            default:
                break;
        }

        mPendingOperation = PendingOperation::None;
        mPendingPath.clear();
        mPendingIsDirectory = false;

        if (mCreateDirectoryPopup) {
            ImGui::OpenPopup("Create Directory");
            mCreateDirectoryPopup = false;
        }

        if (ImGui::BeginPopupModal(
            "Create Directory",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
            ImGui::InputText("Directory Name", &mNewDirectoryName);

            if (ImGui::Button("Create", ImVec2(120, 0))) {
                if (!mNewDirectoryName.empty()) {
                    std::string path = mCreateDirectoryParent;

                    if (!path.empty() && path.back() != '/')
                        path += '/';

                    path += mNewDirectoryName;

                    auto& vfs = mEditorInstance.GetVFS();
                    vfs.CreateDir(path);

                    Refresh();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        mWindowOpen = open;
        ImGui::End();
    }
}
