#pragma once

#include "engine/renderer.hpp"
#include "engine/gameinfo.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/assets/textures.hpp"

namespace CE::UI {
    void DrawDebugUI(CE::Renderer::IRenderer& renderer, CE::Assets::Textures::TextureManager& texman,  CE::GameInfo& gameinfo,
                    Input::Keyboard& kbmanger, Input::Mouse& msmanager);
}
