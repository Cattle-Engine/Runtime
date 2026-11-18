#pragma once

#include "ce_runtime/accs/system.hpp"

namespace CE::ACCS::Systems {
    class MeshRenderer : public ISystem {
        public:
            void Update(float dt, FrameUpdateType type, World& world, ACCS& accs) override;
    };
}