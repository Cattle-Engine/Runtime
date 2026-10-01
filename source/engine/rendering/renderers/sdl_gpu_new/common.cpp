#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    int SDLGPURenderer::Shutdown([[maybe_unused]] SDL_Window* window) {
        ResetFrameState();
        return 0;
    }

    void SDLGPURenderer::ChangeCameraPos2D(float x, float y, float zoom) {
        mCamera2D = Camera2D{x, y, zoom};
        Setup2DCamera();
    }

    void SDLGPURenderer::ChangeCameraPos3D(const Transform3D& transform) {
        mCamera3DState.position = transform.position;
        mCamera3DState.rotation = transform.rotation;
        mCamera3DState.useTarget = false;
        Setup3DCamera();
    }

    void SDLGPURenderer::SetCamera3D(const Camera3D& camera) {
        mCamera3DState = camera;
        Setup3DCamera();
    }

    void SDLGPURenderer::SetClearColour(float r, float g, float b, float a) {
        mClearColour = SDL_FColor{r, g, b, a};
    }

    void SDLGPURenderer::SetRenderSize(glm::vec2 size) {
        if (size.x <= 0.0f) {
            size.x = 1.0f;
        }
        if (size.y <= 0.0f) {
            size.y = 1.0f;
        }

        pRenderSize = size;
        Setup2DCamera();
        Setup3DCamera();
    }

    // TODO: Implement the resource, draw, shader, mesh, and ImGui paths below.
    // These are deliberately inert until the SDL_GPU renderer owns their resource wrappers.
    Texture* SDLGPURenderer::LoadTex([[maybe_unused]] const char* path) { return nullptr; }
    Texture* SDLGPURenderer::CreateTextureFromData([[maybe_unused]] int width, [[maybe_unused]] int height,
                                                   [[maybe_unused]] const void* pixels,
                                                   [[maybe_unused]] TextureFormat format, [[maybe_unused]] int pitch,
                                                   [[maybe_unused]] TextureFilter filter, [[maybe_unused]] TextureWrap wrap,
                                                   [[maybe_unused]] TextureUploadBatch* batch) { return nullptr; }
    void SDLGPURenderer::DrawTex([[maybe_unused]] CE::Renderer::Texture* texture, [[maybe_unused]] float x, [[maybe_unused]] float y,
                                 [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] Colour colour,
                                 [[maybe_unused]] float rotation, [[maybe_unused]] TextureFlip flip) {}
    void SDLGPURenderer::DrawTexUV([[maybe_unused]] CE::Renderer::Texture* texture, [[maybe_unused]] float x, [[maybe_unused]] float y,
                                   [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] float u0,
                                   [[maybe_unused]] float v0, [[maybe_unused]] float u1, [[maybe_unused]] float v1,
                                   [[maybe_unused]] Colour colour, [[maybe_unused]] float rotation,
                                   [[maybe_unused]] TextureFlip flip) {}
    void SDLGPURenderer::UnloadTex([[maybe_unused]] CE::Renderer::Texture* texture) {}
    void SDLGPURenderer::DrawTriangle([[maybe_unused]] float x0, [[maybe_unused]] float y0, [[maybe_unused]] float x1,
                                      [[maybe_unused]] float y1, [[maybe_unused]] float x2, [[maybe_unused]] float y2,
                                      [[maybe_unused]] uint8_t r, [[maybe_unused]] uint8_t g, [[maybe_unused]] uint8_t b,
                                      [[maybe_unused]] uint8_t a, [[maybe_unused]] float rotation) {}
    void SDLGPURenderer::DrawRectLines([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float w,
                                       [[maybe_unused]] float h, [[maybe_unused]] float thickness,
                                       [[maybe_unused]] uint8_t r, [[maybe_unused]] uint8_t g, [[maybe_unused]] uint8_t b,
                                       [[maybe_unused]] uint8_t a) {}
    void SDLGPURenderer::DrawCircleLines([[maybe_unused]] float cx, [[maybe_unused]] float cy,
                                         [[maybe_unused]] float radius, [[maybe_unused]] int segments,
                                         [[maybe_unused]] float thickness, [[maybe_unused]] uint8_t r,
                                         [[maybe_unused]] uint8_t g, [[maybe_unused]] uint8_t b,
                                         [[maybe_unused]] uint8_t a) {}
    void SDLGPURenderer::DrawCircle([[maybe_unused]] float cx, [[maybe_unused]] float cy, [[maybe_unused]] float radius,
                                    [[maybe_unused]] int segments, [[maybe_unused]] uint8_t r, [[maybe_unused]] uint8_t g,
                                    [[maybe_unused]] uint8_t b, [[maybe_unused]] uint8_t a) {}
    void SDLGPURenderer::DrawLine([[maybe_unused]] float x1, [[maybe_unused]] float y1, [[maybe_unused]] float x2,
                                  [[maybe_unused]] float y2, [[maybe_unused]] float thickness,
                                  [[maybe_unused]] uint8_t r, [[maybe_unused]] uint8_t g, [[maybe_unused]] uint8_t b,
                                  [[maybe_unused]] uint8_t a) {}
    Texture* SDLGPURenderer::GetErrorTexture() { return nullptr; }
    void* SDLGPURenderer::GetNativeTextureHandle([[maybe_unused]] CE::Renderer::Texture* texture) { return nullptr; }
    int SDLGPURenderer::Debug_GetVertCount() { return static_cast<int>(mVertexCount); }
    int SDLGPURenderer::Debug_GetIndexCount() { return static_cast<int>(mIndexCount); }
    int SDLGPURenderer::Debug_GetTexIndexCount() { return 0; }
    int SDLGPURenderer::Debug_GetTexVertCount() { return 0; }
    Camera2D* SDLGPURenderer::GetCamera() { return &mCamera2D; }
    void SDLGPURenderer::SetVSync([[maybe_unused]] bool setting) {}
    CE::Renderer::Shader* SDLGPURenderer::CreateShaderProgram() { return nullptr; }
    CE::Renderer::Shader* SDLGPURenderer::LoadShader([[maybe_unused]] const char* path,
                                                      [[maybe_unused]] int fragmentSamplerCount) { return nullptr; }
    bool SDLGPURenderer::LoadShaderStage([[maybe_unused]] CE::Renderer::Shader* shaderProgram,
                                         [[maybe_unused]] const char* path, [[maybe_unused]] ShaderStage stage,
                                         [[maybe_unused]] int samplerCount) { return false; }
    bool SDLGPURenderer::UseDefaultShaderStage([[maybe_unused]] CE::Renderer::Shader* shaderProgram,
                                               [[maybe_unused]] ShaderStage stage) { return false; }
    bool SDLGPURenderer::CompileShaderProgram([[maybe_unused]] CE::Renderer::Shader* shaderProgram) { return false; }
    void SDLGPURenderer::UnloadShader([[maybe_unused]] CE::Renderer::Shader* shader) {}
    void SDLGPURenderer::BindShader([[maybe_unused]] CE::Renderer::Shader* shader) {}
    void SDLGPURenderer::UnbindShader() {}
    void SDLGPURenderer::SetShaderFloat([[maybe_unused]] const char* name, [[maybe_unused]] float value) {}
    void SDLGPURenderer::SetShaderVec2([[maybe_unused]] const char* name, [[maybe_unused]] float x,
                                       [[maybe_unused]] float y) {}
    void SDLGPURenderer::SetShaderVec3([[maybe_unused]] const char* name, [[maybe_unused]] float x,
                                       [[maybe_unused]] float y, [[maybe_unused]] float z) {}
    void SDLGPURenderer::SetShaderVec4([[maybe_unused]] const char* name, [[maybe_unused]] float x,
                                       [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] float w) {}
    void SDLGPURenderer::SetShaderMat4([[maybe_unused]] const char* name, [[maybe_unused]] const float* mat4) {}
    void SDLGPURenderer::SetShaderInt([[maybe_unused]] const char* name, [[maybe_unused]] int value) {}
    void SDLGPURenderer::SetShaderTexture([[maybe_unused]] const char* name, [[maybe_unused]] CE::Renderer::Texture* texture,
                                          [[maybe_unused]] int slot) {}
    GPUMesh* SDLGPURenderer::CreateGPUMesh([[maybe_unused]] MeshData& mesh) { return nullptr; }
    void SDLGPURenderer::DestroyGPUMesh([[maybe_unused]] GPUMesh* mesh) {}
    void SDLGPURenderer::DrawMesh([[maybe_unused]] GPUMesh* mesh, [[maybe_unused]] Material& material,
                                  [[maybe_unused]] const Transform3D& transform, [[maybe_unused]] bool errorTex) {}
    void SDLGPURenderer::DrawMeshMat4([[maybe_unused]] GPUMesh* mesh, [[maybe_unused]] Material& material,
                                      [[maybe_unused]] const glm::mat4& transform, [[maybe_unused]] bool errorTex) {}
    void SDLGPURenderer::ImGuiStartFrame() {}
    void SDLGPURenderer::ImGuiEndFrame([[maybe_unused]] SDL_Window* window) {}
}
