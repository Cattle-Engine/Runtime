#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDLGPURenderer::Draw2DQuad(
        const glm::vec3& position,
        const glm::vec2& size,
        const glm::vec4& colour,
        Texture* texture,
        float rotation) {

        detail::RenderCommand command;
        command.type = detail::RenderCommand::Type::Draw2D;
        command.data = detail::RenderCommand::SpriteDrawCommand {
            .texture = texture,
            .position = position,
            .size = size,
            .colour = colour,
            .rotation = rotation
        };

        mRenderCommands.push_back(command);
    }

    void SDLGPURenderer::DrawRect(
        float x, 
        float y, 
        float w, 
        float h, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        float rotation
    ) {

    }
}