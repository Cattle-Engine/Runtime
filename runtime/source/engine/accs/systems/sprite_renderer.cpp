#include <algorithm>

#include "engine/accs/systems/sprite_renderer.hpp"
#include "engine/accs/components/2d_rendering.hpp"
#include "engine/accs/components/transform.hpp"
#include "engine/accs/system.hpp"
#include "engine/accs/accs.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::ACCS::Systems {
    void SpriteRenderer::Update([[maybe_unused]] float dt, FrameUpdateType type, World& world, ACCS& accs) {
        if (type != FrameUpdateType::Render2D) return;

        accs.world.Each<Components::Sprite2D, Components::Transform>(
            [this](Components::Sprite2D& sprite_2d, Components::Transform& transform) {
                mEntries.push_back({&sprite_2d, &transform.transform_2d});
            }
        );

        std::ranges::stable_sort(
            mEntries,
            [](const Entry& a, const Entry& b) {
                return a.sprite->rendering_priority > b.sprite->rendering_priority;
            }
        );

        for (const auto& entry : mEntries) {
            accs.dependencies.renderer.DrawSprite(
                entry.sprite->texture.GetTexture(), 
                entry.transform->position.x,
                entry.transform->position.y,
                entry.sprite->texture_width,
                entry.sprite->texture_height,
                entry.sprite->colour,
                entry.transform->rotation,
                entry.sprite->texture_flip,
                entry.transform->scale
            );
        }

        mEntries.clear();
    }
}