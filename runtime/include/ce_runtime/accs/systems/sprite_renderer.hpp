#include "ce_runtime/accs/system.hpp"
#include "ce_runtime/rendering/renderer.hpp"
#include "ce_runtime/accs/components/2d_rendering.hpp"

namespace CE::ACCS::Systems {
    class SpriteRenderer : public ISystem {
        public:
            void Update(float dt, FrameUpdateType type, World& world, ACCS& accs) override;
        private:
            struct Entry {
                Components::Sprite2D* sprite = nullptr;
                Renderer::Transform2D* transform = nullptr;
            };

            std::vector<Entry> mEntries{};
    };
}