#pragma once

#include <cstdint>

#include "engine/accs/world.hpp"

namespace CE::ACCS {
    enum class FrameUpdateType : uint8_t {
        None     = 0,
        Logic    = 1 << 0,
        Render2D = 1 << 1,
        Render3D = 1 << 2,
        ImGui    = 1 << 3
    };

    constexpr FrameUpdateType operator|(FrameUpdateType lhs, FrameUpdateType rhs) {
        return static_cast<FrameUpdateType>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
        );
    }

    constexpr FrameUpdateType operator&(FrameUpdateType lhs, FrameUpdateType rhs) {
        return static_cast<FrameUpdateType>(
            static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
        );
    }

    constexpr bool HasFlag(FrameUpdateType flags, FrameUpdateType flag) {
        return (flags & flag) == flag;
    }
    
    class ACCS;

    class ISystem {
        public:
            virtual ~ISystem() = default;

            virtual void Update(float dt, FrameUpdateType type, World& world, ACCS& accs) = 0;
    };
}