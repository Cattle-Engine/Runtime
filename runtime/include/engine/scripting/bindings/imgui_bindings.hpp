#pragma once

#include "engine/rendering/renderer.hpp"
#include <string>
#include <imgui.h>

namespace CE::Scripting::Bindings {
    using Colour = CE::Renderer::Colour;
}

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

    inline int InputInt(const std::string& label, int value) {
        ::ImGui::InputInt(label.c_str(), &value);
        return value;
    }

    inline float InputFloat(const std::string& label, float value) {
        ::ImGui::InputFloat(label.c_str(), &value);
        return value;
    }

    inline int SliderInt(const std::string& label, int value, int min, int max) {
        ::ImGui::SliderInt(label.c_str(), &value, min, max);
        return value;
    }

    inline float SliderFloat(const std::string& label, float value, float min, float max) {
        ::ImGui::SliderFloat(label.c_str(), &value, min, max);
        return value;
    }

    // ImGui treats min >= max as "no limits".
    inline int DragInt(const std::string& label, int value, float speed = 1.0f, int min = 0, int max = 0) {
        ::ImGui::DragInt(label.c_str(), &value, speed, min, max);
        return value;
    }

    inline float DragFloat(const std::string& label, float value, float speed = 1.0f, float min = 0.0f, float max = 0.0f) {
        ::ImGui::DragFloat(label.c_str(), &value, speed, min, max);
        return value;
    }

    // Vector inputs: values go in by value, results come back via out params.
    // Out params are always written, so scripts can assign them unconditionally.
    inline bool InputFloat2(const std::string& label, float x, float y, float& outX, float& outY) {
        float v[2] = { x, y };
        const bool changed = ::ImGui::InputFloat2(label.c_str(), v);
        outX = v[0]; outY = v[1];
        return changed;
    }

    inline bool InputFloat3(const std::string& label, float x, float y, float z,
                            float& outX, float& outY, float& outZ) {
        float v[3] = { x, y, z };
        const bool changed = ::ImGui::InputFloat3(label.c_str(), v);
        outX = v[0]; outY = v[1]; outZ = v[2];
        return changed;
    }

    inline bool DragFloat2(const std::string& label, float x, float y, float& outX, float& outY,
                           float speed = 1.0f, float min = 0.0f, float max = 0.0f) {
        float v[2] = { x, y };
        const bool changed = ::ImGui::DragFloat2(label.c_str(), v, speed, min, max);
        outX = v[0]; outY = v[1];
        return changed;
    }

    inline bool DragFloat3(const std::string& label, float x, float y, float z,
                           float& outX, float& outY, float& outZ,
                           float speed = 1.0f, float min = 0.0f, float max = 0.0f) {
        float v[3] = { x, y, z };
        const bool changed = ::ImGui::DragFloat3(label.c_str(), v, speed, min, max);
        outX = v[0]; outY = v[1]; outZ = v[2];
        return changed;
    }

    inline bool SliderFloat2(const std::string& label, float x, float y, float& outX, float& outY,
                             float min, float max) {
        float v[2] = { x, y };
        const bool changed = ::ImGui::SliderFloat2(label.c_str(), v, min, max);
        outX = v[0]; outY = v[1];
        return changed;
    }

    inline bool SliderFloat3(const std::string& label, float x, float y, float z,
                             float& outX, float& outY, float& outZ, float min, float max) {
        float v[3] = { x, y, z };
        const bool changed = ::ImGui::SliderFloat3(label.c_str(), v, min, max);
        outX = v[0]; outY = v[1]; outZ = v[2];
        return changed;
    }

    namespace Detail {
        inline void ToArray(const CE::Renderer::Colour& c, float (&out)[4]) {
            out[0] = c.r; out[1] = c.g; out[2] = c.b; out[3] = c.a;
        }

        inline CE::Renderer::Colour FromArray(const float (&in)[4], const CE::Renderer::Colour& original) {
            CE::Renderer::Colour c = original;
            c.r = in[0]; c.g = in[1]; c.b = in[2]; c.a = in[3];
            return c;
        }
    } // namespace Detail

    // Alpha is left untouched.
    inline CE::Renderer::Colour ColourEdit3(const std::string& label, CE::Renderer::Colour colour) {
        float v[4]; Detail::ToArray(colour, v);
        ::ImGui::ColorEdit3(label.c_str(), v, ImGuiColorEditFlags_Float);
        return Detail::FromArray(v, colour);
    }

    inline CE::Renderer::Colour ColourEdit4(const std::string& label, CE::Renderer::Colour colour) {
        float v[4];
        Detail::ToArray(colour, v);

        ::ImGui::ColorEdit4(label.c_str(), v, ImGuiColorEditFlags_Float);

        return Detail::FromArray(v, colour);
    }
    
    inline CE::Renderer::Colour ColourPicker4(const std::string& label, CE::Renderer::Colour colour) {
        float v[4]; Detail::ToArray(colour, v);
        ::ImGui::ColorPicker4(label.c_str(), v);
        return Detail::FromArray(v, colour);
    }

    // Small coloured swatch; returns true when clicked.
    inline bool ColourButton(const std::string& id, CE::Renderer::Colour colour) {
        return ::ImGui::ColorButton(id.c_str(), ImVec4(colour.r, colour.g, colour.b, colour.a));
    }
} // namespace CE::Scripting::Bindings::ImGui
