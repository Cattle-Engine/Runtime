#include <imgui.h>
#include <imgui_stdlib.h>

#include "engine/ui/utils.hpp"
#include "engine/ui/angelscript_debugger_window.hpp"

namespace CE::UI {
    namespace {
        void DrawStatCard(const char *label, asUINT value, float width = 150.0f) {
            ImGui::BeginChild(label, ImVec2(width, 75.0f), true);

            ImGui::TextDisabled("%s", label);

            ImGui::Spacing();

            ImGui::Text("%u", value);

            ImGui::EndChild();
        }
    }

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

        if (ImGui::Button("Create function breakpoint")) {
            ImGui::OpenPopup("Create Function Breakpoint");
        }

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

        ImGui::SameLine();

        if (ImGui::Button("Create file breakpoint")) {
            ImGui::OpenPopup("Create File Breakpoint");
        }

        if (ImGui::BeginPopupModal("Create File Breakpoint", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            static std::string file_path;
            static int line;
            static bool show_error_text;

            ImGui::InputText("File name", &file_path);
            ImGui::InputInt("Line", &line);

            if (show_error_text) {
                ImGui::Text("Could not find file in loaded scripts");
            }

            if (ImGui::Button("Create")) {
                for (const auto& file : mRuntime.GetScriptSectionNames()) {
                    if (file == file_path) {
                        show_error_text = false;
                        break;
                    }

                    show_error_text = true;
                }

                if (!show_error_text) mDebugger->AddFileBreakPoint(file_path, line);
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                file_path = "";
                line = 0;
                show_error_text = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete breakpoint") && mSelectedBreakpoint) {
            if (mSelectedBreakpoint->function) {
                mDebugger->RemoveFuncBreakPoint(mSelectedBreakpoint->name);
            } else {
                mDebugger->RemoveFileBreakPoint(mSelectedBreakpoint->name, mSelectedBreakpoint->line);
            }
        }
    }

    void AngelscriptDebuggerWindow::DrawDebuggerOutputTab() {
        ImGui::BeginChild(
            "ConsoleOutput",
            ImVec2(0, 0),
            ImGuiChildFlags_Borders
        );

        std::string output = mDebugger->GetOutputLog();

        ImGui::InputTextMultiline(
            "##Console",
            &output,
            ImVec2(-1, -1),
            ImGuiInputTextFlags_ReadOnly
        );

        ImGui::EndChild();
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

    void AngelscriptDebuggerWindow::DrawCallStackTab() {
        const auto callstack = mDebugger->GetCallStack();

        if (ImGui::BeginTable(
            "Callstack",
            3,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable
        )) {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Location");
            ImGui::TableSetupColumn("Function");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < callstack.size(); i++) {
                const auto& frame = callstack[i];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%zu", i);

                ImGui::TableNextColumn();
                ImGui::Text(
                    "%s:%d",
                    frame.file.c_str(),
                    frame.line
                );

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(frame.function.c_str());
            }

            ImGui::EndTable();
        }
    }

    void AngelscriptDebuggerWindow::DrawStatisticsTab() {
        auto statistics = mDebugger->GetGCStats();
        ImGui::Text("Garbage Collector");
        ImGui::Separator();
        ImGui::Spacing();

        DrawStatCard("Current Size", statistics.CurrentSize);
        ImGui::SameLine();

        DrawStatCard("New Objects", statistics.NewObjects);
        ImGui::SameLine();

        DrawStatCard("Total Detected", statistics.TotalDetected);

        DrawStatCard("Destructions", statistics.TotalDestructions);
        ImGui::SameLine();

        DrawStatCard("New Destructions", statistics.TotalNewDestructions);
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
                if (mDebugger) {
                    ImGui::BeginDisabled(!mDebugger->IsPaused());

                    if (ImGui::Button("Continue")) {
                        mDebugger->Continue();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Step Into")) {
                        mDebugger->StepInto();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Step Over")) {
                        mDebugger->StepOver();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Step Out")) {
                        mDebugger->StepOut();
                    }
                    ImGui::EndDisabled();
                    ImGui::Separator();

                    if (ImGui::BeginTabBar("angelscript_debugger_tab_bar_main")) {
                        if (ImGui::BeginTabItem("Debugger output")) {
                            DrawDebuggerOutputTab();
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Breakpoints")) {
                            DrawBreakPointsTab(); 
                            ImGui::EndTabItem();
                        }
                    
                        if (ImGui::BeginTabItem("Loaded scripts")) {
                            DrawLoadedScriptsTab();
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Callstack")) {
                            DrawCallStackTab();
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Statistics")) {
                            DrawStatisticsTab();
                            ImGui::EndTabItem();
                        }
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::End();
        }
    }
}