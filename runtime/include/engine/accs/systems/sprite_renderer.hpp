#include "engine/accs/system.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/accs/components/2d_rendering.hpp"

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