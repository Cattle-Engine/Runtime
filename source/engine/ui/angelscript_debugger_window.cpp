#include <imgui.h>
#include <imgui_stdlib.h>

#include "engine/ui/utils.hpp"
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

                bool selected = mSelectedBreakpoint &&
                                mSelectedBreakpoint->name == breakpoint.name &&
                                mSelectedBreakpoint->line == breakpoint.line;

                if (ImGui::Selectable(
                        breakpoint.name.c_str(),
                        selected,
                        ImGuiSelectableFlags_SpanAllColumns)) {

                    mSelectedBreakpoint = breakpoint;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%d", breakpoint.line);

                ImGui::TableNextColumn();
                ImGui::Text("%s", breakpoint.function ? "Yes" : "No");
            }

            ImGui::EndTable();
        }

        Utils::SpaceSep();

        bool create_function_popup_open = ImGui::IsPopupOpen("Create Function Breakpoint");
        ImGui::BeginDisabled(create_function_popup_open);
        if (ImGui::Button("Create function breakpoint")) {
            ImGui::OpenPopup("Create Function Breakpoint");
        }
        ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("Create Function Breakpoint", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            static std::string function_name;

            ImGui::InputText("Function Name", &function_name);

            if (ImGui::Button("Create")) {
                mDebugger->AddFuncBreakPoint(function_name);
                function_name.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                function_name.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        bool create_file_breakpoint_popup_open = ImGui::IsPopupOpen("Create File Breakpoint");

        ImGui::BeginDisabled(create_file_breakpoint_popup_open);

        ImGui::SameLine();

        if (ImGui::Button("Delete breakpoint") && mSelectedBreakpoint) {
            if (mSelectedBreakpoint->function) {
                mDebugger->RemoveFuncBreakPoint(mSelectedBreakpoint->name);
            } else {
                mDebugger->RemoveFileBreakPoint(mSelectedBreakpoint->name, mSelectedBreakpoint->line);
            }
        }
    }

    void AngelscriptDebuggerWindow::DrawLoadedScriptsTab() {
        const auto& files = mRuntime.GetScriptSectionNames();

        static int selected_file = -1;

        ImGui::BeginChild(
            "script_files",
            ImVec2(250.0f, 0.0f),
            ImGuiChildFlags_Borders
        );

        for (int i = 0; i < static_cast<int>(files.size()); ++i) {
            if (ImGui::Selectable(files[i].c_str(), selected_file == i)) {
                selected_file = i;
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild(
            "script_source",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_Borders
        );

        if (selected_file >= 0 &&
            selected_file < static_cast<int>(mCachedScriptCodeSource.size())) {

            ImGui::TextUnformatted(files[selected_file].c_str());
            ImGui::Separator();

            ImGui::BeginChild(
                "source",
                ImVec2(0.0f, 0.0f),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_HorizontalScrollbar
            );

            ImGui::TextUnformatted(
                mCachedScriptCodeSource[selected_file].c_str()
            );

            ImGui::EndChild();
        } else {
            ImGui::TextUnformatted("Select a script");
        }

        ImGui::EndChild();
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
                    if (ImGui::BeginTabItem("Breakpoints")) {
                        DrawBreakPointsTab(); 
                        ImGui::EndTabItem();
                    }
                 
                    if (ImGui::BeginTabItem("Loaded scripts")) {
                        DrawLoadedScriptsTab();
                        ImGui::EndTabItem();
                    }
                }

                ImGui::EndTabBar();
            }
            ImGui::End();
        }
    }
}