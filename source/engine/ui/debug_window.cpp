#include "imgui/imgui.h"

#include "engine/ui/debug_window.hpp"
#include "engine/renderer.hpp"
#include "engine/gameinfo.hpp"
#include "engine/assets/textures.hpp"

namespace CE::UI {
    static void DrawRenderTab(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman, CE::GameInfo& gameinfo) {
        ImGui::Text("Renderer Name: %s", gameinfo.rendererName.c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Vertex Count: %d", renderer.Debug_GetVertCount());
        ImGui::Text("Texture Vertex Count: %d", renderer.Debug_GetTexVertCount());
        ImGui::Text("Index Count: %d", renderer.Debug_GetIndexCount());
        ImGui::Text("Texture Index Count: %d", renderer.Debug_GetTexIndexCount());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Total textures loaded: %d", texman.Debug_LoadedTexturesCount());
        ImGui::Text("Loaded textures no error count: %d", texman.Debug_LoadedTexturesNoError());
        ImGui::Text("Loaded textures error count: %d", texman.Debug_LoadedTexturesError());
    }

    void DrawDebugUI(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman
                    , CE::GameInfo& gameinfo) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cattle Debug");
        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Renderer")) {
                DrawRenderTab(renderer, texman, gameinfo);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
