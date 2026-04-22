#include "imgui/imgui.h"

#include "engine/ui/debug_window.hpp"
#include "engine/ui/utils.hpp"
#include "engine/renderer.hpp"
#include "engine/gameinfo.hpp"
#include "engine/assets/textures.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"

namespace CE::UI {
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

    void DrawRenderTab(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman, CE::GameInfo& gameinfo) {
        ImGui::Text("Renderer Name: %s", gameinfo.rendererName.c_str());
        
        Utils::SpaceSep();

        ImGui::Text("Vertex Count: %d", renderer.Debug_GetVertCount());
        ImGui::Text("Texture Vertex Count: %d", renderer.Debug_GetTexVertCount());
        ImGui::Text("Index Count: %d", renderer.Debug_GetIndexCount());
        ImGui::Text("Texture Index Count: %d", renderer.Debug_GetTexIndexCount());

        Utils::SpaceSep();

        ImGui::Text("Total textures loaded: %d", texman.Debug_LoadedTexturesCount());
        ImGui::Text("Loaded textures no error count: %d", texman.Debug_LoadedTexturesNoError());
        ImGui::Text("Loaded textures error count: %d", texman.Debug_LoadedTexturesError());
    }

    void DrawDebugUI(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman,  CE::GameInfo& gameinfo,
                    Input::Keyboard& kbmanger, Input::Mouse& msmanager) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
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

            if (ImGui::BeginTabItem("Renderer")) {
                DrawRenderTab(renderer, texman, gameinfo);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
