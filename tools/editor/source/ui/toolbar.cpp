#include <algorithm>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_messagebox.h>
#include "imgui.h"

#include "ce_editor/editor.hpp"
#include "ce_editor/ui/toolbar.hpp"

namespace CE::Editor::UI {
    namespace {
        void ExportLogsCallback(void* userdata, const char* const* filelist, int filter) {
            if (!filelist || !filelist[0]) return;

            auto instance = static_cast<EditorInstance*>(userdata);

            auto result = instance->LoadProject(filelist[0]);
            if (!result) {
                SDL_ShowSimpleMessageBox(
                    SDL_MESSAGEBOX_ERROR,
                    "CE Editor - Error", 
                    result.GetError().message.c_str(), 
                    nullptr
                );
            }
        }
    }

    void MainViewPortToolBar::Draw() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if(ImGui::MenuItem("Load Project")) {
                    SDL_ShowOpenFolderDialog(
                        ExportLogsCallback, 
                        static_cast<void*>(&mInstance), 
                        nullptr, 
                        nullptr,
                        false
                    );
                }

                ImGui::Separator();

                if(ImGui::BeginMenu("Recent Projects")) {
                    bool empty = std::all_of(
                        mInstance.GetRecentProjects().begin(),
                        mInstance.GetRecentProjects().end(),
                        [](const std::string& project) {
                            return project.empty();
                        }
                    );
                    if (empty) {
                        ImGui::TextDisabled("No recent projects");
                    } else {
                        for (const auto& file : mInstance.GetRecentProjects()) {
                            if (file.empty()) continue;
                            if (ImGui::MenuItem(file.c_str())) {
                                mInstance.LoadProject(file);
                            }
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if(ImGui::MenuItem("Quit")) {
                    mInstance.Exit();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {


                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                for (auto& window : mInstance.GetUIWindows()) {
                    bool open = window->IsWindowOpen();
                    if (ImGui::MenuItem(window->GetWindowName().c_str(), nullptr, &open)) {
                        window->SetWindowOpen(open);
                    }
                }
                ImGui::EndMenu();
            }

            // Push buttons to the right
            const float buttonSize = ImGui::GetFrameHeight();
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            const float totalWidth = (buttonSize * 2.0f) + spacing;

            ImGui::SameLine(ImGui::GetWindowWidth() - totalWidth - spacing);

            // Undo
            if (ImGui::ImageButton(
                "##Undo",
                static_cast<ImTextureRef>(
                    mInstance.GetThemeManager().GetNativeIconTexture(Icon::Undo)
                ),
                ImVec2(18.0f, 18.0f)
            )) {
                // Redo action
            }

            ImGui::SameLine();

            if (ImGui::ImageButton(
                "##Redo",
                static_cast<ImTextureRef>(
                    mInstance.GetThemeManager().GetNativeIconTexture(Icon::Redo)
                ),
                ImVec2(18.0f, 18.0f)
            )) {
                // Redo action
            }
            ImGui::EndMainMenuBar();
        }
    }
}