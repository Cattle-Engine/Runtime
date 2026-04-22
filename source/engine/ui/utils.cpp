#include "imgui/imgui.h" 

namespace CE::UI::Utils {
    void SpaceSep() {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
}