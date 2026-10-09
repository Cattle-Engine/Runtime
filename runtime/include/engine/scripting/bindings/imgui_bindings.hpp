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

    inline void NewLine() {
        ::ImGui::NewLine();
    }

    inline void Spacing() {
        ::ImGui::Spacing();
    }

    inline void Dummy(float width, float height) {
        ::ImGui::Dummy(ImVec2(width, height));
    }

    inline void Indent(float width = 0.0f) {
        ::ImGui::Indent(width);
    }

    inline void Unindent(float width = 0.0f) {
        ::ImGui::Unindent(width);
    }

    inline bool BeginChild(const std::string& id, float width, float height) {
        return ::ImGui::BeginChild(id.c_str(), ImVec2(width, height));
    }

    inline void EndChild() {
        ::ImGui::EndChild();
    }

    inline bool CollapsingHeader(const std::string& label) {
        return ::ImGui::CollapsingHeader(label.c_str());
    }

    inline bool TreeNode(const std::string& label) {
        return ::ImGui::TreeNode(label.c_str());
    }

    inline void TreePop() {
        ::ImGui::TreePop();
    }

    inline bool Selectable(const std::string& label, bool selected = false) {
        return ::ImGui::Selectable(label.c_str(), selected);
    }

    inline bool BeginCombo(const std::string& label, const std::string& preview) {
        return ::ImGui::BeginCombo(label.c_str(), preview.c_str());
    }

    inline void EndCombo() {
        ::ImGui::EndCombo();
    }

    inline void TextDisabled(const std::string& text) {
        ::ImGui::TextDisabled("%s", text.c_str());
    }

    inline void BulletText(const std::string& text) {
        ::ImGui::BulletText("%s", text.c_str());
    }

    inline void LabelText(const std::string& label, const std::string& text) {
        ::ImGui::LabelText(label.c_str(), "%s", text.c_str());
    }

    inline void OpenPopup(const std::string& id) {
        ::ImGui::OpenPopup(id.c_str());
    }

    inline bool BeginPopup(const std::string& id) {
        return ::ImGui::BeginPopup(id.c_str());
    }

    inline void EndPopup() {
        ::ImGui::EndPopup();
    }

    inline bool BeginPopupModal(const std::string& title) {
        return ::ImGui::BeginPopupModal(title.c_str());
    }

    inline bool BeginMenu(const std::string& label) {
        return ::ImGui::BeginMenu(label.c_str());
    }

    inline void EndMenu() {
        ::ImGui::EndMenu();
    }

    inline bool MenuItem(const std::string& label) {
        return ::ImGui::MenuItem(label.c_str());
    }

    inline bool Checkbox(const std::string& label, bool checked) {
        ::ImGui::Checkbox(label.c_str(), &checked);
        return checked;
    }
} // namespace CE::Scripting::Bindings::ImGui
