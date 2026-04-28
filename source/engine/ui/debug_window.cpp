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

    void DrawInstanceTab(GameInfo& gameinfo, Instance& instance) {
        ImGui::Text("InstanceID: %i", instance.GetInstanceID());

        if (ImGui::Button("Quit instance")) {
            instance.Exit();   
        }

        Utils::SpaceSep();
        
        if (ImGui::CollapsingHeader("Gameinfo")) {
            ImGui::Text("Game name: %s", gameinfo.gameNameString.c_str());
            ImGui::Text("Game version: %s", gameinfo.gameVersionString.c_str());
            ImGui::Text("Window title: %s", gameinfo.windowTitle.c_str());
            ImGui::Text("Window size: %i x %i", gameinfo.windowWidth, gameinfo.windowHeight);
            ImGui::Text("VSync: %s", gameinfo.enableVSync ? "Enabled" : "Disabled");
            ImGui::Text("Fullscreen: %s", gameinfo.fullscreen ? "Yes" : "No");
            ImGui::Text("Resizable Window: %s", gameinfo.resizableWindow ? "Yes" : "No");
        }
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

    void DrawRendererTab(
        CE::Renderer::IRenderer& renderer,
        Settings::SettingsInfo settings,
        Assets::Textures::TextureManager& texman,
        Assets::Fonts::FontManager& fontman
    ) 
    {
        Renderer::Camera2D* camera = renderer.GetCamera();

        ImGui::Text("Current renderer: %s", settings.rendererName.c_str());

        CE::UI::Utils::SpaceSep();

        ImGui::Text("Camera");
        ImGui::Text("Position: %f X, %f Y", camera->x, camera->y);
        ImGui::Text("Zoom: %f", camera->zoom);

        CE::UI::Utils::SpaceSep();

        ImGui::Text("Geometry");
        ImGui::Spacing();

        ImGui::Text("Vertex Count: %d", renderer.Debug_GetVertCount());
        ImGui::Text("Texture Vertex Count: %d", renderer.Debug_GetTexVertCount());
        ImGui::Text("Index Count: %d", renderer.Debug_GetIndexCount());
        ImGui::Text("Texture Index Count: %d", renderer.Debug_GetTexIndexCount());

        CE::UI::Utils::SpaceSep();

        ImGui::Text("Textures");
        ImGui::Spacing();

        ImGui::Text("Total loaded: %d", texman.Debug_LoadedTexturesCount());
        ImGui::Text("No error: %d", texman.Debug_LoadedTexturesNoError());
        ImGui::Text("Errors: %d", texman.Debug_LoadedTexturesError());

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Fonts", ImGuiTreeNodeFlags_DefaultOpen)) {

            auto defaultFont = fontman.Debug_GetDefaultFontName();
            ImGui::Text("Default Font: %s", defaultFont.c_str());

            auto atlases = fontman.Debug_GetAtlases();
            ImGui::Text("Atlases: %zu", atlases.size());

            CE::UI::Utils::SpaceSep();

            static char familyBuf[64] = {};
            static int sizeBuf = 16;

            ImGui::Text("Atlas Viewer");

            ImGui::InputText("Family", familyBuf, sizeof(familyBuf));
            ImGui::InputInt("Size", &sizeBuf);

            if (sizeBuf < 1) sizeBuf = 1;

            auto* tex = fontman.Debug_GetAtlasTex(familyBuf, sizeBuf);

            if (tex) {
                ImGui::Text("Atlas Preview:");
                void* nativeTexture = renderer.GetNativeTextureHandle(tex);
                if (nativeTexture) {
                    ImGui::Image((ImTextureID)(intptr_t)nativeTexture, ImVec2(256, 256));
                } else {
                    ImGui::TextDisabled("Atlas texture is not available for ImGui preview");
                }
            } else {
                ImGui::TextDisabled("No atlas found");
            }

            CE::UI::Utils::SpaceSep();

            if (ImGui::TreeNode("Atlas List")) {

                for (const auto& a : atlases) {

                    ImGui::PushID(a.key.c_str());

                    if (ImGui::TreeNode(a.key.c_str())) {

                        ImGui::Text("Family: %s", a.familyName.c_str());
                        ImGui::Text("Size: %d", a.fontSize);
                        ImGui::Text("Glyphs: %zu", a.glyphCount);

                        ImGui::Text("Atlas: %dx%d", a.atlasWidth, a.atlasHeight);
                        ImGui::Text("Pen: %d, %d", a.penX, a.penY);
                        ImGui::Text("RowH: %d", a.rowH);

                        ImGui::Text("Texture: %s", a.hasTexture ? "Yes" : "No");
                        ImGui::Text("Dirty: %s", a.dirty ? "Yes" : "No");

                        ImGui::Text("Memory: %.2f KB",
                            a.estimatedMemoryBytes / 1024.0f);

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        }
    }

    void DrawDebugUI(
        CE::Renderer::IRenderer& renderer,
        CE::Assets::Textures::TextureManager& texman,
        CE::Assets::Fonts::FontManager& fontman,
        CE::GameInfo& gameinfo,
        CE::Settings::SettingsManager& settings,
        Input::Keyboard& kbmanger,
        CE::Instance& instance,
        Input::Mouse& msmanager,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        ImGui::SetNextWindowSize(ImVec2(487, 386), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cattle Debug");

        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Instance")) {
                DrawInstanceTab(gameinfo, instance);
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

            if (ImGui::BeginTabItem("Renderer")) {
                DrawRendererTab(renderer, settings.Settings, texman, fontman);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
