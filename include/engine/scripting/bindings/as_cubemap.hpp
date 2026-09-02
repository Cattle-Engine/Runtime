#pragma once

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Scripting::Bindings {
    using TexRef = CE::Renderer::Resources::TextureRef;
    using TexHandle = CE::Renderer::Resources::TextureHandle;
    
    struct ASCubemap {
        // not exposed to AS
        CE::Renderer::CubeMap cubemap;

        // also not exposed to as
        TexRef face_right_ref;
        TexRef face_left_ref;
        TexRef face_top_ref;
        TexRef face_bottom_ref;
        TexRef face_front_ref;
        TexRef face_back_ref;

        TexHandle face_right_handle;
    };
}