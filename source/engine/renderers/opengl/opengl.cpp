#include "engine/renderers/opengl.hpp"

#include "engine/common/error_box.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/gameinfo.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>

#include "third_party/glad/glad.h"

#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <vector>

namespace CE::Renderers::OpenGL {
    static SDL_GLContext gContext = nullptr;
    static GLuint gProgram = 0;
    static GLuint gVao = 0;
    static GLuint gVbo = 0;
    static GLuint gEbo = 0;

    static GLint uMvpLoc = -1;
    static GLint uTexLoc = -1;
    static GLint uColorLoc = -1;
    static GLint uUvTransformLoc = -1;


    void preWindowCreation() {
        // Set some openGL settings for sdl
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // 3.3 because the engine is 2d and we dont need fancy shit
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); 
    }

    void Init(SDL_Window* window) {
        gContext = SDL_GL_CreateContext(window);

        if (!gContext) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL] Failed to create OpenGL context: {}", SDL_GetError());
            ShowError("[OpenGL] Failed to create OpenGL context");
            std::exit(5);
        }

        SDL_GL_MakeCurrent(window, gContext);

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL] Failed to initialize GLAD");
            ShowError("[OpenGL] Failed to initialize GLAD");
            std::exit(6);
        }

        if (CE::GameInfo::enableVSync) {
            SDL_GL_SetSwapInterval(1);
        }

        const char* vsSrc = R"GLSL(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aUv;

            uniform mat4 uMVP;
            uniform vec4 uUvTransform;

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

        GLuint vs = Utils::CompileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = Utils::CompileShader(GL_FRAGMENT_SHADER, fsSrc);

        if (!vs || !fs) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL] Failed to compile required shaders");
            ShowError("[OpenGL] Failed to compile required shaders");
            std::exit(1);
        }

        gProgram = Utils::LinkProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        if (!gProgram) {
            CE::Log(CE::LogLevel::Fatal, "[OpenGL] Failed to link shader program");
            ShowError("[OpenGL] Failed to link shader program");
            std::exit(1);
        }

        glUseProgram(gProgram);

        uTexLoc = glGetUniformLocation(gProgram, "uTex");
        uMvpLoc = glGetUniformLocation(gProgram, "uMVP");
        uColorLoc = glGetUniformLocation(gProgram, "uColor");
        uUvTransformLoc = glGetUniformLocation(gProgram, "uUvTransform");
        
        glUniform1i(uTexLoc, 0);

        glUseProgram(0);

        const float vertices[] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
        };

        const unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &gVao);
        glGenBuffers(1, &gVbo);
        glGenBuffers(1, &gEbo);

        glBindVertexArray(gVao);
        glBindBuffer(GL_ARRAY_BUFFER, gVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    std::shared_ptr<CE::Renderers::Texture> Load(const char* path) {
        std::vector<uint8_t> bytes;
        if (!Utils::ReadAllBytesFromVFS(path, bytes)) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] Failed to read texture: {}", path ? path : "(null)");
            return nullptr;
        }

        SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
        if (!io) {
            CE::Log(CE::LogLevel::Error, "[OpenGL renderer] SDL_IOFromConstMem failed: {}", SDL_GetError());
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // FIX consistency
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            converted->w,
            converted->h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            converted->pixels
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        SDL_DestroySurface(converted);

        SDL_CloseIO(io);

        allocatedTextures.push_back(texId);

        auto texture = std::make_shared<CE::Renderers::Texture>();
        texture->handle = (void*)(uintptr_t)texId;
        texture->width = converted->w;
        texture->height = converted->h;
        texture->format = CE::Renderers::TextureFormat::RGBA8;

        return texture;
    }

    void BeginFrame(SDL_Window* window) {
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
    }

    void EndFrame(SDL_Window* window) {
        SDL_GL_SwapWindow(window);
    }

}
