#pragma once

#include "engine/rendering/renderer.hpp"

namespace CE::Scripting::Bindings {
    struct ASCubemap {
        // not exposed to AS
        CE::Renderer::CubeMap cubemap;
    };
}