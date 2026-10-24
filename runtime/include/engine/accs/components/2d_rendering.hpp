#pragma once

#include "engine/accs/component.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::ACCS::Components {
    class Sprite2D : public IComponent {
        public:
            // NOLINTBEGIN
            // stop clang-tidy complaining about name styling
            Renderer::Resources::TextureHandle texture;
            // higher priority's draw first
            int rendering_priority = 0;
            // tint of the sprite, white means no tint
            Renderer::Colour colour = Renderer::Colour::White;
            Renderer::TextureFlip texture_flip = Renderer::TextureFlip::None;

            float texture_width = texture.GetTexturwWidth();
            float texture_height = texture.GetTextureHeight();
            // NOLINTEND
    };
}