#include <array>
#include <cstring>
#include <unordered_map>

#include "imgui/imgui.h"

#include "engine/ui/debug_window.hpp"
#include "engine/ui/utils.hpp"
#include "engine/renderer.hpp"
#include "engine/common/gameinfo.hpp"
#include "engine/assets/textures.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"

namespace CE::UI {
    namespace {
        struct SettingsTabState {
            std::array<char, 501> rendererBuffer{};
            bool synced = false;
        };

        SettingsTabState& GetSettingsTabState(CE::Settings::SettingsManager& settings) {
            static std::unordered_map<CE::Settings::SettingsManager*, SettingsTabState> states;
            return states[&settings];
        }
    }

    void DrawGameinfoTab(GameInfo& gameinfo) {
        ImGui::Text("Game name: %s", gameinfo.gameNameString.c_str());
        ImGui::Text("Game version: %s", gameinfo.gameVersionString.c_str());

        Utils::SpaceSep();

        ImGui::Text("Window title: %s", gameinfo.windowTitle.c_str());
        ImGui::Text("Window size: %i x %i", gameinfo.windowWidth, gameinfo.windowHeight);
        ImGui::Text("VSync: %s", gameinfo.enableVSync ? "Enabled" : "Disabled");
        ImGui::Text("Fullscreen: %s", gameinfo.fullscreen ? "Yes" : "No");
        ImGui::Text("Resizable Window: %s", gameinfo.resizableWindow ? "Yes" : "No");
    }

    void DrawInputTab(Input::Keyboard& kbmanger, Input::Mouse& msmanager) {
        ImGui::Text("Keyboard");
        ImGui::Spacing();
        ImGui::Text("Currently held keys: %s", kbmanger.GetPressedKeysString().c_str());

        Utils::SpaceSep();

        ImGui::Text("Mouse");
        ImGui::Spacing();
        ImGui::Text("Mouse posX: %i", msmanager.GetX());
        ImGui::Text("Mouse posY: %i", msmanager.GetY());
        ImGui::Text("Mouse delta posX: %i", msmanager.GetDeltaX());
        ImGui::Text("Mouse delta posY: %i", msmanager.GetDeltaY());
        ImGui::Spacing();
        ImGui::Text("Mouse wheelX: %i", msmanager.GetWheelX());
        ImGui::Text("Mouse wheelY: %i", msmanager.GetWheelY());
    }

    void DrawPerformanceTab(
        CE::Renderer::IRenderer& renderer,
        CE::Assets::Textures::TextureManager& texman,
        const CE::Settings::SettingsInfo& settings,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        ImGui::Text("Performance");
        ImGui::Spacing();

        ImGui::Text("FPS: %d", fps);
        ImGui::Text("Frame Time (ms): %.3f", frameTime);
        ImGui::Text("Delta Time (s): %.6f", deltaTime);

        static float fpsHistory[100] = {};
        static int offset = 0;

        fpsHistory[offset] = static_cast<float>(fps);
        offset = (offset + 1) % 100;

        ImGui::PlotLines("FPS History", fpsHistory, 100, 0, nullptr, 0.0f, 300.0f, ImVec2(0, 80));

        Utils::SpaceSep();

        ImGui::Text("Renderer: %s", settings.rendererName.c_str());

        Utils::SpaceSep();

        ImGui::Text("Geometry");
        ImGui::Spacing();

        ImGui::Text("Vertex Count: %d", renderer.Debug_GetVertCount());
        ImGui::Text("Texture Vertex Count: %d", renderer.Debug_GetTexVertCount());
        ImGui::Text("Index Count: %d", renderer.Debug_GetIndexCount());
        ImGui::Text("Texture Index Count: %d", renderer.Debug_GetTexIndexCount());

        Utils::SpaceSep();

        ImGui::Text("Textures");
        ImGui::Spacing();

        ImGui::Text("Total loaded: %d", texman.Debug_LoadedTexturesCount());
        ImGui::Text("No error: %d", texman.Debug_LoadedTexturesNoError());
        ImGui::Text("Errors: %d", texman.Debug_LoadedTexturesError());
    }

    void DrawSettingsTab(CE::Settings::SettingsManager& settings) {
        auto& s = settings.Settings;
        auto& state = GetSettingsTabState(settings);

        ImGui::Text("Window");
        ImGui::Spacing();

        ImGui::InputInt("Width", &s.windowWidth);
        ImGui::InputInt("Height", &s.windowHeight);

        ImGui::Checkbox("Fullscreen", &s.fullscreen);
        ImGui::Checkbox("VSync", &s.enableVSync);

        Utils::SpaceSep();

        ImGui::Text("Performance");
        ImGui::Spacing();

        ImGui::SliderInt("Max FPS", &s.maxFPS, 30, 240);
        ImGui::Text("Note: This is ignored if VSync is on and\nFPS is locked to display refresh rate");

        Utils::SpaceSep();

        ImGui::Text("Renderer");
        ImGui::Spacing();

        if (!state.synced) {
            std::strncpy(state.rendererBuffer.data(), s.rendererName.c_str(), state.rendererBuffer.size() - 1);
            state.rendererBuffer[state.rendererBuffer.size() - 1] = '\0';
            state.synced = true;
        }

        ImGui::PushID(&settings);
        ImGui::InputText("Renderer", state.rendererBuffer.data(), state.rendererBuffer.size());
        ImGui::PopID();

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            s.rendererName = state.rendererBuffer.data();
        }

        ImGui::Text("Supported renderers: Metal, DX12, Vulkan");
        ImGui::Text("Note: To change renderer you need to close engine and reopen.");

        Utils::SpaceSep();

        ImGui::Text("Settings path: %s", settings.GetSettingPath().c_str());

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to copy to clipboard");

            if (ImGui::IsMouseClicked(0)) {
                SDL_SetClipboardText(settings.GetSettingPath().c_str());
            }
        }

        if (ImGui::Button("Save & apply")) {
            settings.FlushSettings();
            settings.ReloadSettings();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reload from disk")) {
            settings.ReloadSettings();

            std::strncpy(state.rendererBuffer.data(), s.rendererName.c_str(), state.rendererBuffer.size() - 1);
            state.rendererBuffer[state.rendererBuffer.size() - 1] = '\0';
            state.synced = true;
        }
    }

    void DrawDebugUI(
        CE::Renderer::IRenderer& renderer,
        CE::Assets::Textures::TextureManager& texman,
        CE::GameInfo& gameinfo,
        CE::Settings::SettingsManager& settings,
        Input::Keyboard& kbmanger,
        Input::Mouse& msmanager,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        ImGui::SetNextWindowSize(ImVec2(487, 386), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cattle Debug");

        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Gameinfo")) {
                DrawGameinfoTab(gameinfo);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input")) {
                DrawInputTab(kbmanger, msmanager);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                DrawSettingsTab(settings);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance")) {
                DrawPerformanceTab(renderer, texman, settings.Settings, fps, deltaTime, frameTime);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}