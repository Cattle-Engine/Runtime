#include "engine/renderers/opengl.hpp"

#include "engine/common/error_box.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>

#include "third_party/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace CE::Renderers::OpenGL {

    static SDL_Window* window = nullptr;
    static SDL_GLContext glContext = nullptr;

    static GLuint program = 0;
    static GLuint vao = 0;
    static GLuint vbo = 0;
    static GLuint ebo = 0;

    static GLint uMvpLoc = -1;
    static GLint uTexLoc = -1;
    static GLint uColorLoc = -1;
    static GLint uUvTransformLoc = -1;

    static glm::mat4 viewProj{ 1.0f };

    static std::vector<GLuint> allocatedTextures;

    static CE::Renderers::Colour drawColour{ 255, 255, 255, 255 };
    static CE::Renderers::Texture whiteTexture{ nullptr, 1, 1, CE::Renderers::TextureFormat::RGBA8 };

    static glm::vec4 ToVec4(const CE::Renderers::Colour& c) {
        return glm::vec4(
            static_cast<float>(c.r) / 255.0f,
            static_cast<float>(c.g) / 255.0f,
            static_cast<float>(c.b) / 255.0f,
            static_cast<float>(c.a) / 255.0f
        );
    }

    static glm::vec4 ComputeUvTransform(const CE::Renderers::Texture* texture, const CE::Renderers::DrawTextureOptions& options) {
        float offX = 0.0f;
        float offY = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;

        if (texture && texture->width > 0 && texture->height > 0 && options.src.w > 0.0f && options.src.h > 0.0f) {
            offX = options.src.x / static_cast<float>(texture->width);
            offY = options.src.y / static_cast<float>(texture->height);
            scaleX = options.src.w / static_cast<float>(texture->width);
            scaleY = options.src.h / static_cast<float>(texture->height);
        }

        const bool flipX = options.flip == CE::Renderers::Flip::Horizontal || options.flip == CE::Renderers::Flip::HorizontalAndVertical;
        const bool flipY = options.flip == CE::Renderers::Flip::Vertical || options.flip == CE::Renderers::Flip::HorizontalAndVertical;

        if (flipX) {
            offX += scaleX;
            scaleX = -scaleX;
        }
        if (flipY) {
            offY += scaleY;
            scaleY = -scaleY;
        }

        return glm::vec4(offX, offY, scaleX, scaleY);
    }

    static void CreateWhiteTexture() {
        if (whiteTexture.handle)
            return;

        GLuint texId = 0;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        const uint32_t pixel = 0xFFFFFFFFu;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
        glBindTexture(GL_TEXTURE_2D, 0);

        allocatedTextures.push_back(texId);

        whiteTexture.handle = reinterpret_cast<void*>(static_cast<uintptr_t>(texId));
        whiteTexture.width = 1;
        whiteTexture.height = 1;
        whiteTexture.format = CE::Renderers::TextureFormat::RGBA8;
    }

    static GLuint CompileShader(GLenum type, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint logLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 1));
            glGetShaderInfoLog(shader, logLen, nullptr, log.data());
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Shader compile failed: {}", log.data());
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    static GLuint LinkProgram(GLuint vs, GLuint fs) {
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

        GLint ok = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint logLen = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 1));
            glGetProgramInfoLog(prog, logLen, nullptr, log.data());
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Program link failed: {}", log.data());
            glDeleteProgram(prog);
            return 0;
        }

        return prog;
    }

    static bool ReadAllBytesFromVFS(const char* virtual_path, std::vector<uint8_t>& out_bytes) {
        out_bytes.clear();
        if (!virtual_path)
            return false;

        auto& vfs = CE::Global::GetVFS();
        uint64_t size64 = 0;
        if (!vfs.GetFileSize(virtual_path, size64))
            return false;

        if (size64 == 0) {
            out_bytes.clear();
            return true;
        }

        if (size64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            return false;

        auto* file = vfs.OpenFile(virtual_path);
        if (!file)
            return false;

        out_bytes.resize(static_cast<size_t>(size64));
        size_t offset = 0;
        while (offset < out_bytes.size()) {
            const size_t n = vfs.ReadFile(file, out_bytes.data() + offset, out_bytes.size() - offset);
            if (n == 0)
                break;
            offset += n;
        }
        vfs.CloseFile(file);

        if (offset != out_bytes.size())
            return false;

        return true;
    }

    void Init(SDL_Window* window) {
        CE::Renderers::OpenGL::window = window;
        if (!window) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Init called with null window");
            ShowError("[OpenGL Renderer] Init called with null window");
            std::exit(1);
        }

        SDL_WindowFlags flags = SDL_GetWindowFlags(window);
        if ((flags & SDL_WINDOW_OPENGL) == 0) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Window missing SDL_WINDOW_OPENGL flag");
            ShowError("[OpenGL Renderer] Window missing SDL_WINDOW_OPENGL flag");
            std::exit(1);
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Failed to create GL context: {}", SDL_GetError());
            ShowError("[OpenGL Renderer] Failed to create GL context");
            std::exit(1);
        }

        if (!SDL_GL_MakeCurrent(window, glContext)) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Failed to make GL context current: {}", SDL_GetError());
            ShowError("[OpenGL Renderer] Failed to make GL context current");
            std::exit(1);
        }

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Failed to load OpenGL via GLAD");
            ShowError("[OpenGL Renderer] Failed to load OpenGL via GLAD");
            std::exit(1);
        }

        SDL_GL_SetSwapInterval(1);

        const char* vsSrc = R"GLSL(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aUv;

            uniform mat4 uMVP;
            uniform vec4 uUvTransform; // (offset.xy, scale.zw)

            out vec2 vUv;

            void main() {
                vUv = uUvTransform.xy + (aUv * uUvTransform.zw);
                gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
            }
        )GLSL";

        const char* fsSrc = R"GLSL(
            #version 330 core
            in vec2 vUv;
            uniform sampler2D uTex;
            uniform vec4 uColor;
            out vec4 FragColor;

            void main() {
                FragColor = texture(uTex, vUv) * uColor;
            }
        )GLSL";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
        if (!vs || !fs) {
            ShowError("[OpenGL Renderer] Failed to compile shaders");
            std::exit(1);
        }

        program = LinkProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        if (!program) {
            ShowError("[OpenGL Renderer] Failed to link shader program");
            std::exit(1);
        }

        uMvpLoc = glGetUniformLocation(program, "uMVP");
        uTexLoc = glGetUniformLocation(program, "uTex");
        uColorLoc = glGetUniformLocation(program, "uColor");
        uUvTransformLoc = glGetUniformLocation(program, "uUvTransform");

        glUseProgram(program);
        glUniform1i(uTexLoc, 0);
        glUseProgram(0);

        // Unit quad in pixel-space; model matrix scales it to size.
        // aPos is [0..1] in X/Y, so scaling becomes exact.
        const float vertices[] = {
            // pos      // uv
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
        };

        const unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        CreateWhiteTexture();
    }

    void BeginFrame() {
        int w = 0;
        int h = 0;
        if (!SDL_GetWindowSizeInPixels(window, &w, &h)) {
            SDL_GetWindowSize(window, &w, &h);
        }
        if (w <= 0 || h <= 0) {
            w = 1;
            h = 1;
        }

        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const auto& cam = CE::Renderers::camera;

        const glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, -1.0f, 1.0f);
        glm::mat4 view{ 1.0f };
        view = glm::scale(view, glm::vec3(cam.zoom, cam.zoom, 1.0f));
        view = glm::translate(view, glm::vec3(-cam.x, -cam.y, 0.0f));
        viewProj = proj * view;
    }

    void EndFrame() {
        SDL_GL_SwapWindow(window);
    }

    std::shared_ptr<CE::Renderers::Texture> Load(const char* path)
    {
        std::vector<uint8_t> bytes;
        if (!ReadAllBytesFromVFS(path, bytes)) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] Failed to read texture from VFS: {}", path ? path : "(null)");
            return nullptr;
        }

        SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
        if (!io) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] Failed to create SDL_IOStream: {}", SDL_GetError());
            return nullptr;
        }

        SDL_Surface* surface = IMG_Load_IO(io, true);
        if (!surface) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] IMG_Load_IO failed: {}", SDL_GetError());
            return nullptr;
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] SDL_ConvertSurface failed: {}", SDL_GetError());
            return nullptr;
        }

        GLuint texId = 0;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        const int texW = converted->w;
        const int texH = converted->h;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            texW,
            texH,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            converted->pixels
        );
        glBindTexture(GL_TEXTURE_2D, 0);

        allocatedTextures.push_back(texId);

        SDL_DestroySurface(converted);

        auto texture = std::make_shared<CE::Renderers::Texture>();
        texture->handle = reinterpret_cast<void*>(static_cast<uintptr_t>(texId));
        texture->width = texW;
        texture->height = texH;
        texture->format = CE::Renderers::TextureFormat::RGBA8;
        return texture;
    }

    void Draw(CE::Renderers::Texture* texture, float x, float y, float w, float h)
    {
        const CE::Renderers::RectF dst{ x, y, w, h };
        CE::Renderers::DrawTextureOptions options;
        CE::Renderers::OpenGL::Draw(texture, dst, options);
    }

    void Draw(CE::Renderers::Texture* texture, const CE::Renderers::RectF& dst, const CE::Renderers::DrawTextureOptions& options) {
        if (!texture || !texture->handle)
            return;

        const GLuint texId = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texture->handle));

        glm::mat4 model{ 1.0f };
        model = glm::translate(model, glm::vec3(dst.x, dst.y, 0.0f));
        model = glm::translate(model, glm::vec3(options.origin.x, options.origin.y, 0.0f));
        model = glm::rotate(model, glm::radians(options.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-options.origin.x, -options.origin.y, 0.0f));
        model = glm::scale(model, glm::vec3(dst.w, dst.h, 1.0f));

        const glm::mat4 mvp = viewProj * model;

        const glm::vec4 color = ToVec4(options.tint);
        const glm::vec4 uvTransform = ComputeUvTransform(texture, options);

        glUseProgram(program);
        glUniformMatrix4fv(uMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4fv(uColorLoc, 1, glm::value_ptr(color));
        glUniform4fv(uUvTransformLoc, 1, glm::value_ptr(uvTransform));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        drawColour = CE::Renderers::Colour{ r, g, b, a };
    }

    void DrawRect(float x, float y, float w, float h) {
        if (w == 0.0f || h == 0.0f)
            return;
        if (w < 0.0f) {
            x += w;
            w = -w;
        }
        if (h < 0.0f) {
            y += h;
            h = -h;
        }

        const CE::Renderers::RectF dst{ x, y, w, h };
        CE::Renderers::DrawTextureOptions options;
        options.tint = drawColour;
        CE::Renderers::OpenGL::Draw(&whiteTexture, dst, options);
    }

    void DrawRectLines(float x, float y, float w, float h, float thickness) {
        if (w == 0.0f || h == 0.0f || thickness <= 0.0f)
            return;
        if (w < 0.0f) {
            x += w;
            w = -w;
        }
        if (h < 0.0f) {
            y += h;
            h = -h;
        }

        thickness = std::min(thickness, std::min(w, h));
        const float innerH = std::max(0.0f, h - (2.0f * thickness));

        // Top
        DrawRect(x, y, w, thickness);
        // Bottom
        DrawRect(x, y + h - thickness, w, thickness);
        // Left
        DrawRect(x, y + thickness, thickness, innerH);
        // Right
        DrawRect(x + w - thickness, y + thickness, thickness, innerH);
    }

    void DrawLine(float x1, float y1, float x2, float y2, float thickness) {
        if (thickness <= 0.0f)
            return;

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float len = std::sqrt((dx * dx) + (dy * dy));
        if (len <= 0.0001f) {
            DrawRect(x1 - (thickness * 0.5f), y1 - (thickness * 0.5f), thickness, thickness);
            return;
        }

        constexpr float Pi = 3.14159265358979323846f;
        const float angleDeg = std::atan2(dy, dx) * (180.0f / Pi);

        CE::Renderers::RectF dst{ x1, y1 - (thickness * 0.5f), len, thickness };
        CE::Renderers::DrawTextureOptions options;
        options.rotation = angleDeg;
        options.origin = CE::Renderers::Vec2{ 0.0f, thickness * 0.5f };
        options.tint = drawColour;
        CE::Renderers::OpenGL::Draw(&whiteTexture, dst, options);
    }

    void Shutdown()
    {
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }

        if (ebo) {
            glDeleteBuffers(1, &ebo);
            ebo = 0;
        }
        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        if (!allocatedTextures.empty()) {
            glDeleteTextures(static_cast<GLsizei>(allocatedTextures.size()), allocatedTextures.data());
            allocatedTextures.clear();
        }
        whiteTexture.handle = nullptr;

        if (glContext) {
            SDL_GL_DestroyContext(glContext);
            glContext = nullptr;
        }
        window = nullptr;
    }
}
