#include <imgui.h>
#include <imgui_stdlib.h>

#include "engine/ui/angelscript_debugger_window.hpp"

namespace CE::UI {
    void AngelscriptDebuggerWindow::OpenWindow() {
        mWindowOpen = true;
    }

    void AngelscriptDebuggerWindow::DrawBreakPointsTab() {
        auto breakpoints_list = mDebugger->GetBreakPoints();

        if (ImGui::BeginTable("Breakpoints", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Line");
            ImGui::TableSetupColumn("Function");
            ImGui::TableHeadersRow();

            for (const auto& breakpoint : breakpoints_list) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%s", breakpoint.name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%d", breakpoint.line);

                ImGui::TableNextColumn();
                ImGui::Text("%s", breakpoint.function ? "Yes" : "No");
            }

            ImGui::EndTable();
        }
    }

    void AngelscriptDebuggerWindow::DrawWindow() {
        if (mWindowOpen) {
            if(ImGui::Begin("Angelscript Debugger", &mWindowOpen)) {
                if (!mDebugger) {
                    ImGui::Text("Debugger system not available");
                    if (ImGui::Button("Close Window")) {
                        mWindowOpen = false;
                    }
                }

                if (ImGui::BeginTabBar("angelscript_debugger_tab_bar_main")) {
                    if (ImGui::BeginTabItem("Breakpoints")) DrawBreakPointsTab();
                }
            }
        }
        ImGui::End();
    }
}