#pragma once

#include "engine/ui/debug_window.hpp"
#include "engine/renderer.hpp"
#include "engine/gameinfo.hpp"
#include "engine/assets/textures.hpp"

namespace CE::UI {
    void DrawDebugUI(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman,  CE::GameInfo& gameinfo);
}
