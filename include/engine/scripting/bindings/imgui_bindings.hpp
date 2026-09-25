#pragma once

#include <string>
#include <imgui.h>

namespace CE::Scripting::Bindings::ImGui {
    inline bool Begin(const std::string& title) { return ::ImGui::Begin(title.c_str()); }
    inline void End() { ::ImGui::End(); }
    inline void Text(const std::string& text) { ::ImGui::TextUnformatted(text.c_str()); }
    inline bool Button(const std::string& label) { return ::ImGui::Button(label.c_str()); }
    inline void Separator() { ::ImGui::Separator(); }
    inline void SameLine() { ::ImGui::SameLine(); }
} // namespace CE::Scripting::Bindings::ImGui
