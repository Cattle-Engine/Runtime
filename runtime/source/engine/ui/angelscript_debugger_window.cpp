#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <sstream>

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

        void DrawDebugValue(const Scripting::ScriptDebugger::DebugValue& value, const std::string& varName = "") {
            const std::string displayName = varName.empty() ? value.name : varName;
            if (value.members.empty()) {
                ImGui::TextUnformatted(displayName.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("=");
                ImGui::SameLine();
                ImGui::Text("%s", value.value.c_str());
            } else {
                if (ImGui::TreeNode(displayName.c_str())) {
                    ImGui::TextDisabled("%s", value.value.c_str());
                    for (const auto& member : value.members) {
                        DrawDebugValue(member);
                    }
                    ImGui::TreePop();
                }
            }
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

            if (ImGui::Button("Create") && !function_name.empty()) {
                mDebugger->AddFuncBreakPoint(function_name);
                function_name.clear();
                ImGui::CloseCurrentPopup();
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
            static bool show_error_text = false;

            ImGui::InputText("File name", &file_path);
            ImGui::InputInt("Line", &line);

            if (show_error_text) {
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Choose a loaded script and a line greater than zero");
            }

            if (ImGui::Button("Create")) {
                const auto& files = mRuntime.GetScriptSectionNames();
                const bool knownFile = std::find(files.begin(), files.end(), file_path) != files.end();
                show_error_text = !knownFile || line <= 0;
                if (!show_error_text) {
                    mDebugger->AddFileBreakPoint(file_path, line);
                    file_path.clear();
                    line = 0;
                    ImGui::CloseCurrentPopup();
                }
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
            mSelectedBreakpoint.reset();
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

        if (const auto location = mDebugger->GetCurrentLocation()) {
            if (mLastDebuggerFile != location->file) {
                const auto it = std::find(files.begin(), files.end(), location->file);
                if (it != files.end()) {
                    selected_file = static_cast<int>(
                        std::distance(files.begin(), it)
                    );
                }

                mLastDebuggerFile = location->file;
            }
        } else {
            mLastDebuggerFile.clear();
        }
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

            const auto breakpoints = mDebugger->GetBreakPoints();
            const auto current = mDebugger->GetCurrentLocation();
            const std::string& selectedFile = files[selected_file];
            std::istringstream source(mCachedScriptCodeSource[selected_file]);
            std::string lineText;
            int lineNumber = 1;

            while (std::getline(source, lineText)) {
                const bool isCurrent = current && current->file == selectedFile && current->line == lineNumber;
                const bool hasBreakpoint = std::any_of(breakpoints.begin(), breakpoints.end(),
                    [&selectedFile, lineNumber](const auto& breakpoint) {
                        return !breakpoint.function && breakpoint.name == selectedFile && breakpoint.line == lineNumber;
                    });

                ImGui::PushID(lineNumber);
                if (ImGui::SmallButton(hasBreakpoint ? "-" : "+")) {
                    if (hasBreakpoint)
                        mDebugger->RemoveFileBreakPoint(files[selected_file], lineNumber);
                    else
                        mDebugger->AddFileBreakPoint(files[selected_file], lineNumber);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("%4d", lineNumber);
                ImGui::SameLine();
                if (isCurrent)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
                ImGui::TextUnformatted(lineText.c_str());
                if (isCurrent)
                    ImGui::PopStyleColor();
                ImGui::PopID();
                ++lineNumber;
            }

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

    void AngelscriptDebuggerWindow::DrawLocalVariablesTab() {
        auto locals = mDebugger->GetLocalVariables();

        if (locals.empty()) {
            ImGui::TextDisabled("No local variables in scope");
            return;
        }

        ImGui::BeginChild("LocalVariables", ImVec2(0, 0), ImGuiChildFlags_Borders);

        for (const auto& var : locals) {
            DrawDebugValue(var);
        }

        ImGui::EndChild();
    }

    void AngelscriptDebuggerWindow::DrawGlobalVariablesTab() {
        auto globals = mDebugger->GetGlobalVariables();

        if (globals.empty()) {
            ImGui::TextDisabled("No global variables");
            return;
        }

        ImGui::BeginChild("GlobalVariables", ImVec2(0, 0), ImGuiChildFlags_Borders);

        for (const auto& var : globals) {
            DrawDebugValue(var);
        }

        ImGui::EndChild();
    }

    void AngelscriptDebuggerWindow::DrawMemberPropertiesTab() {
        auto members = mDebugger->GetMemberProperties();

        if (members.name.empty() && members.value.empty() && members.members.empty()) {
            ImGui::TextDisabled("No object context (not in a member function)");
            return;
        }

        ImGui::BeginChild("MemberProperties", ImVec2(0, 0), ImGuiChildFlags_Borders);

        if (!members.members.empty()) {
            ImGui::TextDisabled("this");
            for (const auto& member : members.members) {
                DrawDebugValue(member);
            }
        } else {
            ImGui::TextDisabled("No members to display");
        }

        ImGui::EndChild();
    }

    void AngelscriptDebuggerWindow::DrawContexts() {
        const auto contexts = mDebugger->GetContexts();
        if (contexts.empty()) {
            ImGui::TextDisabled("No script contexts are attached");
            return;
        }

        ImGui::TextUnformatted("Script contexts");
        ImGui::SameLine();
        ImGui::TextDisabled("Select one to inspect it; uncheck Pause to let it run through breakpoints.");

        if (ImGui::BeginChild("ScriptContextsScroll", ImVec2(0.0f, 100.0f), ImGuiChildFlags_Borders)) {
            if (ImGui::BeginTable("ScriptContexts", 3,
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp)) {

                ImGui::TableSetupColumn("Context");
                ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Pause", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableHeadersRow();

                for (const auto& context : contexts) {
                    ImGui::PushID(static_cast<int>(context.id));

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(
                        context.name.c_str(),
                        context.selected))
                        mDebugger->SelectContext(context.id);

                    ImGui::TableNextColumn();
                    if (context.paused)
                        ImGui::TextColored(
                            ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                            "Paused");
                    else
                        ImGui::TextDisabled("Idle");

                    ImGui::TableNextColumn();
                    bool pauseAtBreakpoints = context.pauseAtBreakpoints;
                    if (ImGui::Checkbox("##pause", &pauseAtBreakpoints))
                        mDebugger->SetContextPauseEnabled(
                            context.id,
                            pauseAtBreakpoints);

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();
    }

    void AngelscriptDebuggerWindow::DrawWindow() {
        const bool is_paused = mDebugger && mDebugger->IsPaused();

        if (is_paused && !mWasPaused)
            mWindowOpen = true;

        mWasPaused = is_paused;

        if (!mWindowOpen) return;

        if (mWindowOpen) {
            if(ImGui::Begin("Angelscript Debugger", &mWindowOpen)) {
                if (!mDebugger) {
                    ImGui::Text("Debugger system not available");
                    if (ImGui::Button("Close Window")) {
                        mWindowOpen = false;
                    }
                }
                if (mDebugger) {
                    DrawContexts();
                    ImGui::Separator();
                    if (const auto location = mDebugger->GetCurrentLocation()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Paused at %s:%d", location->file.c_str(), location->line);
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%s)", location->function.c_str());
                    } else {
                        ImGui::TextDisabled("Running - execution stops at breakpoints.");
                    }
                    ImGui::BeginDisabled(!mDebugger->IsSelectedContextPaused());

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

                        if (ImGui::BeginTabItem("Local Variables")) {
                            DrawLocalVariablesTab();
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Global Variables")) {
                            DrawGlobalVariablesTab();
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Members")) {
                            DrawMemberPropertiesTab();
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
