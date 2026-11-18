#include <algorithm>

#include "ce_runtime/accs/systems/sprite_renderer.hpp"
#include "ce_runtime/accs/components/2d_rendering.hpp"
#include "ce_runtime/accs/components/transform.hpp"
#include "ce_runtime/accs/system.hpp"
#include "ce_runtime/accs/accs.hpp"
#include "ce_runtime/rendering/renderer.hpp"

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