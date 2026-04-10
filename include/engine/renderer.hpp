#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

enum class RendererBackend {
    Software,
    OpenGL,
    DirectX,
    Metal,
    Vulkan,
    None
};

namespace CE::Renderer {
    class IRenderer;

    inline RendererBackend renderer = RendererBackend::None;
    inline IRenderer* currentRenderer = nullptr;
    inline std::string rendererName = "None";

    IRenderer* CreateRenderer(RendererBackend backend);
    
    struct Camera2D {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    enum class TextureFormat {
        RGBA8,
        RGB8,
    };

    struct Colour { // BRITISH SPELLING HEEEEEEEEEHHEHEHE
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };

    struct Vertex {
        float x, y, z;   
        Uint8 r, g, b, a;
    };

    struct Texture {
        void* handle;    
        int width;
        int height;
        TextureFormat format;
        RendererBackend backend;
    };

    class IRenderer {
        public:
            virtual void PreWinInit() = 0;
            virtual void Init(SDL_Window* window) = 0;
            virtual void Shutdown() = 0;

            virtual void BeginFrame(SDL_Window* window) = 0;
            virtual void EndFrame(SDL_Window* window) = 0;

           // virtual void ChangeCameraPos(float X, float Y, float zoom) = 0;
           // virtual Texture* Load(const char* path) = 0;
           // virtual void Draw(Texture* texture, float x, float y, float w, float h, Colour colour) = 0;
           // virtual void DrawRect(float x, float y, float w, float h) = 0;
           // virtual void DrawRectLines(float x, float y, float w, float h, float thickness = 1.0f) = 0;
           // virtual void DrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f) = 0;
           // virtual void DrawCircleLines(float cx, float cy, float radius, int segments = 32, float thickness = 1.0f) = 0;

            virtual ~IRenderer() = default;
    };
}

namespace CE::Renderer::Utils {
    SDL_GPUShader* LoadShader( 
        SDL_GPUDevice* device,
        const std::string& shaderfilename,
        Uint32 samplercount,
        Uint32 uniformbuffercount,
        Uint32 storagebuffercount,
        Uint32 storagetexturecount,
        const std::string& basePath = "/shaders/"
    );
    glm::mat4 GetView(const Camera2D& cam);
    glm::mat4 GetProjection(float width, float height);
    glm::mat4 GetCameraMatrix(const Camera2D& cam, float w, float h);
}
