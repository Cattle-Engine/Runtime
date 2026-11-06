
#include <imgui.h>
#include <SDL3/SDL.h>
#include <unordered_set>
#include <algorithm>
#include <SDL3/SDL_dialog.h>
#include <filesystem>
#include <fstream>

#include "engine/ui/logs_window.hpp"
#include "engine/common/tracelog.hpp"

#include <string>
#include <vector>

namespace CE::UI {
    namespace {
        void ExportLogsCallback(void* userdata, const char* const* filelist, int filter) {
            if (!filelist || !filelist[0])
                return;

            if (!CECommon::DumpLogsToFile(filelist[0]))
                SDL_Log("Failed to export logs: %s", filelist[0]);
        }
    }

    void LogWindow::DrawWindow() {
        if (!mWindowOpen)
            return;

        static ImGuiTextFilter filter;
        static int level_filter = -1;
        static bool auto_scroll = true;
        static std::unordered_set<size_t> selected_logs;
        static int selection_anchor = -1;

        ImGui::SetNextWindowSize(ImVec2(750, 450), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Logs", &mWindowOpen)) {
            const auto& logs = CE::GetLogs();

            // ImGui::SameLine(ImGui::GetWindowWidth() - 190.0f);
            if (ImGui::SmallButton("Clear")) {
                CECommon::ClearLogs();
                selected_logs.clear();
                selection_anchor = -1;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &auto_scroll);
            ImGui::SameLine();
            if (ImGui::SmallButton("Export")) {
                static const SDL_DialogFileFilter filters[] = {
                    { "Log Files", "log" },
                    { "Text Files", "txt" },
                    { "All Files", "*" },
                };

                SDL_ShowSaveFileDialog(
                    ExportLogsCallback,
                    nullptr,
                    nullptr,
                    filters,
                    IM_ARRAYSIZE(filters),
                    "cattle_engine.log"
                );
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(%zu entries)", logs.size());
            ImGui::Separator();
            ImGui::SameLine();

            // Search
            ImGui::TextUnformatted("Search");
            ImGui::SameLine();

            const float search_width = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(search_width);

            filter.Draw("##Search", 0.0f);

            // Level filters
            const char* levels[] = {
                "All",
                "Debug",
                "Info",
                "Warning",
                "Error",
                "Fatal"
            };

            ImGui::Spacing();

            for (int i = 0; i < IM_ARRAYSIZE(levels); ++i) {
                if (i > 0)
                    ImGui::SameLine();

                bool selected = level_filter == (i - 1);

                if (selected)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                if (ImGui::SmallButton(levels[i]))
                    level_filter = i - 1;

                if (selected)
                    ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // Log entries
            const float footer_height = ImGui::GetFrameHeightWithSpacing();

            std::vector<size_t> visible_logs;

            for (size_t i = 0; i < logs.size(); ++i) {
                const auto& [level, message] = logs[i];

                if (level_filter != -1 && static_cast<int>(level) != level_filter)
                    continue;

                if (!filter.PassFilter(message.c_str()))
                    continue;

                visible_logs.push_back(i);
            }

            if (ImGui::BeginChild("LogEntries", ImVec2(0, -footer_height), ImGuiChildFlags_Borders)) {
                if (ImGui::BeginTable("LogTable", 2,
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_BordersInnerV |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_ScrollY |
                        ImGuiTableFlags_ScrollX |
                        ImGuiTableFlags_SizingFixedFit
                    )) {

                    ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                    ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthFixed, 1000.0f);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < visible_logs.size(); ++i) {
                        const size_t log_index = visible_logs[i];
                        const auto& [level, message] = logs[log_index];

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        ImGui::PushID(static_cast<int>(log_index));

                        bool selected = selected_logs.contains(log_index);

                        if (ImGui::Selectable(
                            "##LogSelect",
                            selected,
                            ImGuiSelectableFlags_SpanAllColumns
                        )) {
                            const auto& io = ImGui::GetIO();

                            if (io.KeyShift && selection_anchor >= 0) {
                                if (!io.KeyCtrl)
                                    selected_logs.clear();

                                const size_t start = std::min(
                                    static_cast<size_t>(selection_anchor), i
                                );

                                const size_t end = std::max(
                                    static_cast<size_t>(selection_anchor), i
                                );

                                for (size_t j = start; j <= end; ++j)
                                    selected_logs.insert(visible_logs[j]);
                            }
                            else if (io.KeyCtrl) {
                                if (selected_logs.contains(log_index))
                                    selected_logs.erase(log_index);
                                else
                                    selected_logs.insert(log_index);

                                selection_anchor = static_cast<int>(i);
                            }
                            else {
                                selected_logs.clear();
                                selected_logs.insert(log_index);
                                selection_anchor = static_cast<int>(i);
                            }
                        }

                        ImGui::SameLine();

                        ImVec4 colour;
                        const char* label = "";

                        switch (level) {
                        case CE::LogLevel::Debug:
                            colour = ImVec4(0.45f, 0.65f, 1.0f, 1.0f);
                            label = "DEBUG";
                            break;

                        case CE::LogLevel::Info:
                            colour = ImVec4(0.35f, 0.75f, 1.0f, 1.0f);
                            label = "INFO";
                            break;

                        case CE::LogLevel::Warn:
                            colour = ImVec4(1.0f, 0.75f, 0.25f, 1.0f);
                            label = "WARN";
                            break;

                        case CE::LogLevel::Error:
                            colour = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
                            label = "ERROR";
                            break;

                        case CE::LogLevel::Fatal:
                            colour = ImVec4(1.0f, 0.15f, 0.15f, 1.0f);
                            label = "FATAL";
                            break;
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, colour);
                        ImGui::TextUnformatted(label);
                        ImGui::PopStyleColor();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(message.c_str());

                        ImGui::PopID();
                    }
                    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                        ImGui::SetScrollHereY(1.0f);

                    ImGui::EndTable();

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                        ImGui::GetIO().KeyCtrl &&
                        ImGui::IsKeyPressed(ImGuiKey_C) &&
                        !selected_logs.empty()) {

                        std::string clipboard;

                        // Preserve original log ordering.
                        for (size_t i = 0; i < logs.size(); ++i) {
                            if (!selected_logs.contains(i))
                                continue;

                            const auto& [level, message] = logs[i];

                            clipboard += message;
                            clipboard += '\n';
                        }

                        SDL_SetClipboardText(clipboard.c_str());
                    }
                }
            }

            ImGui::EndChild();

            ImGui::Separator();

            ImGui::TextDisabled("Cattle Engine | %zu log entries", logs.size());
        }

        ImGui::End();
    }
}
