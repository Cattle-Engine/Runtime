#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include "third_party/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "engine/renderers.hpp"

namespace CE::Renderers::OpenGL {
    static CE::Renderers::Texture whiteTexture{ nullptr, 1, 1, CE::Renderers::TextureFormat::RGBA8, RendererBackend::OpenGL};
    static std::vector<GLuint> allocatedTextures;

    void Init(SDL_Window* window);
    void BeginFrame(SDL_Window* window);
    void EndFrame(SDL_Window* window);
    std::shared_ptr<CE::Renderers::Texture> Load(const char* path);
    void Draw(CE::Renderers::Texture* texture, float x, float y, float w, float h);
    void Draw(CE::Renderers::Texture* texture, const CE::Renderers::RectF& dst, const CE::Renderers::DrawTextureOptions& options);
    void Unload(CE::Renderers::Texture* texture);

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void DrawRect(float x, float y, float w, float h);
    void DrawRectLines(float x, float y, float w, float h, float thickness = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f);
    void Shutdown();
}

namespace CE::Renderers::OpenGL::Utils {
    static void CreateWhiteTexture();
    static GLuint CompileShader(GLenum type, const char* src);
    static GLuint LinkProgram(GLuint vs, GLuint fs);
    GLuint GetGLTexture(const Texture& t);
    static bool ReadAllBytesFromVFS(const char* virtual_path, std::vector<uint8_t>& out_bytes);
}
