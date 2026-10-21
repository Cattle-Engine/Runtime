#pragma once

#include <cstdint>

#include "engine/accs/world.hpp"

namespace CE::ACCS {
    enum class FrameUpdateType : uint8_t {
        None,
        Logic,
        Render2D,
        Render3D,
        ImGui,
        Count
    };

    class ISystem {
        public:
            virtual ~ISystem() = default;

            virtual void Update(float dt, FrameUpdateType type, World& world) = 0;
    };
}