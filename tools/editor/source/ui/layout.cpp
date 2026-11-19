#include <imgui.h>
#include <imgui_internal.h>

#include "ce_editor/ui/layout.hpp"

namespace CE::Editor::UI {
    void UILayout::Draw() {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("EditorDockspaceHost", nullptr, flags);

        ImGui::PopStyleVar(3);

        mDockspaceID = ImGui::GetID("EditorDockspace");

        ImGui::DockSpace(
            mDockspaceID,
            ImVec2(0.0f, 0.0f),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        if (!mInitialized)
            InitialiseDefaultLayout();

        ImGui::End();
    }

    void UILayout::InitialiseDefaultLayout() {
        ImGui::DockBuilderRemoveNode(mDockspaceID);

        ImGui::DockBuilderAddNode(
            mDockspaceID,
            ImGuiDockNodeFlags_DockSpace
        );

        ImGui::DockBuilderSetNodeSize(
            mDockspaceID,
            ImGui::GetMainViewport()->WorkSize
        );

        ImGuiID left;
        ImGuiID center = mDockspaceID;

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Left,
            0.22f,
            &left,
            &center
        );

        ImGui::DockBuilderDockWindow("Project Browser", left);

        ImGui::DockBuilderFinish(mDockspaceID);

        mInitialized = true;
    }

    void UILayout::Reset() {
        mInitialized = false;
    }
}