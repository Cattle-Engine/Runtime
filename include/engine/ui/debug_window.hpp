#pragma once

#include "engine/renderer.hpp"
#include "engine/common/gameinfo.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/assets/textures.hpp"
#include "engine/settings.hpp"

namespace CE::UI {
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
    );
}
