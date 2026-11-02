#pragma once

#include "engine/accs/component.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"
#include "engine/accs/serialization_context.hpp"

#include "engine/accs/accs.hpp"

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

            float texture_width = 0.0f;
            float texture_height = 0.0f;
            // NOLINTEND

            bool Serialize(CECommon::FS::BinaryWriter& writer, const SaveContext& ctx) const {
                const std::string texture_path = ctx.GetDependencies().texture_manager.GetTexturePath(texture);

                return writer.WriteSizedString(texture_path) &&
                    writer.WriteU32(static_cast<uint32_t>(rendering_priority)) &&
                    writer.WriteF32(texture_width) &&
                    writer.WriteF32(texture_height) &&
                    writer.WriteF32(colour.r) &&
                    writer.WriteF32(colour.g) &&
                    writer.WriteF32(colour.b) &&
                    writer.WriteF32(colour.a) &&
                    writer.WriteU8(static_cast<uint8_t>(texture_flip));
            }

            bool Deserialize(CECommon::FS::BinaryReader& reader, const LoadContext& ctx) {
                std::string texture_path;
                uint32_t priority = 0;
                float width = 0.0f;
                float height = 0.0f;
                Renderer::Colour loaded_colour{};
                uint8_t flip = 0;

                if (!reader.ReadSizedString(texture_path) ||
                    !reader.ReadU32(priority) ||
                    !reader.ReadF32(width) ||
                    !reader.ReadF32(height) ||
                    !reader.ReadF32(loaded_colour.r) ||
                    !reader.ReadF32(loaded_colour.g) ||
                    !reader.ReadF32(loaded_colour.b) ||
                    !reader.ReadF32(loaded_colour.a) ||
                    !reader.ReadU8(flip)) {
                    return false;
                }

                if (flip > static_cast<uint8_t>(Renderer::TextureFlip::None)) {
                    return false;
                }

                auto& texture_manager = ctx.GetDependencies().texture_manager;

                auto loaded_texture = texture_path.empty()
                    ? Renderer::Resources::kInvalidHandle
                    : texture_manager.Load(std::move(texture_path));

                texture = std::move(loaded_texture);
                rendering_priority = static_cast<int>(priority);
                texture_width = width;
                texture_height = height;
                colour = loaded_colour;
                texture_flip = static_cast<Renderer::TextureFlip>(flip);

                return true;
            }
    };
}