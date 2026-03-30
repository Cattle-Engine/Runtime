#include <SDL3/SDL.h>
#include "engine/core.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/renderers.hpp"

#include <cmath>

#define CE_ENABLE_SOFTWARE_RENDER
#define CE_ENABLE_OPENGL_RENDER

#if defined(CE_ENABLE_SOFTWARE_RENDER)
#include "engine/renderers/software.hpp"
#endif

#if defined(CE_ENABLE_OPENGL_RENDER)
#include "engine/renderers/opengl.hpp"
#endif

namespace CE::Renderers {
    void Init(SDL_Window* window) {
        switch (CE::Renderers::renderer) {
            case RendererBackend::Software:
            #ifdef CE_ENABLE_SOFTWARE_RENDER 
                CE::Renderers::Software::Init(window);
            #else
                CE::Log(CE::LogLevel::Error, "[Renderer Manager] Software renderer is not supported");
                ShowError("Unsupported renderer: Software");
                std::exit(1);      
            #endif
            break;

            case RendererBackend::OpenGL:
            #ifdef CE_ENABLE_OPENGL_RENDER
                CE::Renderers::OpenGL::Init(window);
            #else
                CE::Log(CE::LogLevel::Error, "[Renderer Manager] OpenGL renderer is not supported");
                ShowError("Unsupported renderer: OpenGL");
                std::exit(1);
            #endif
            break;

            case RendererBackend::None:
                CE::Log(CE::LogLevel::Error, "[Renderer Manager] No renderer selected");
            break;

            default:
                return;
            break;
        }
    }
    
    void BeginFrame() {
        switch (CE::Renderers::renderer) {
            case RendererBackend::None:
                return;
            break;

            case RendererBackend::Software:
                #ifdef CE_ENABLE_SOFTWARE_RENDER
                    CE::Renderers::Software::BeginFrame();
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] Software renderer is not supported");
                    ShowError("Unsupported renderer: Software");
                    std::exit(1);
                #endif
                    break;

            case RendererBackend::OpenGL:
                #ifdef CE_ENABLE_OPENGL_RENDER
                    CE::Renderers::OpenGL::BeginFrame();
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] OpenGL renderer is not supported");
                    ShowError("Unsupported renderer: OpenGL");
                    std::exit(1);
                #endif
                    break;

                default:
                    return;
                break;
        }
    }

    void EndFrame() {
        switch (CE::Renderers::renderer) {
            case RendererBackend::None:
                return;
            break;

            case RendererBackend::Software:
                #ifdef CE_ENABLE_SOFTWARE_RENDER
                    CE::Renderers::Software::EndFrame();
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] Software renderer is not supported");
                    ShowError("Unsupported renderer: Software");
                    std::exit(1);
                #endif
                    break;

            case RendererBackend::OpenGL:
                #ifdef CE_ENABLE_OPENGL_RENDER
                    CE::Renderers::OpenGL::EndFrame();
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] OpenGL renderer is not supported");
                    ShowError("Unsupported renderer: OpenGL");
                    std::exit(1);
                #endif
                    break;

                default:
                    return;
                break;
        }
    }

    std::shared_ptr<Texture> LoadTexture(const char* path) {
        switch (CE::Renderers::renderer) {
            case RendererBackend::None:
                return nullptr;

            case RendererBackend::Software:
                #ifdef CE_ENABLE_SOFTWARE_RENDER
                    return CE::Renderers::Software::Load(path);
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] Software renderer is not supported");
                    ShowError("Unsupported renderer: Software");
                    std::exit(1);
                #endif
                    break;

            case RendererBackend::OpenGL:
                #ifdef CE_ENABLE_OPENGL_RENDER
                    return CE::Renderers::OpenGL::Load(path);
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] OpenGL renderer is not supported");
                    ShowError("Unsupported renderer: OpenGL");
                    std::exit(1);
                #endif
                    break;

                default:
                    return nullptr;
        }
    }

    void Draw(Texture* texture, float x, float y, float w, float h) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::Draw(texture, x, y, w, h);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::Draw(texture, x, y, w, h);
                break;

            default:
                break;
        }
    }

    void Draw(Texture* texture, const RectF& dst, const DrawTextureOptions& options) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::Draw(texture, dst, options);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::Draw(texture, dst, options);
                break;

            default:
                break;
        }
    }

    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::SetColor(r, g, b, a);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::SetColor(r, g, b, a);
                break;

            default:
                break;
        }
    }

    void DrawRect(float x, float y, float w, float h) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::DrawRect(x, y, w, h);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::DrawRect(x, y, w, h);
                break;

            default:
                break;
        }
    }

    void DrawRectLines(float x, float y, float w, float h, float thickness) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::DrawRectLines(x, y, w, h, thickness);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::DrawRectLines(x, y, w, h, thickness);
                break;

            default:
                break;
        }
    }

    void DrawLine(float x1, float y1, float x2, float y2, float thickness) {
        switch (renderer) {
            case RendererBackend::Software:
                CE::Renderers::Software::DrawLine(x1, y1, x2, y2, thickness);
                break;

            case RendererBackend::OpenGL:
                CE::Renderers::OpenGL::DrawLine(x1, y1, x2, y2, thickness);
                break;

            default:
                break;
        }
    }

    void DrawCircleLines(float cx, float cy, float radius, int segments, float thickness) {
        if (radius <= 0.0f || segments < 3 || thickness <= 0.0f)
            return;

        constexpr float Pi = 3.14159265358979323846f;
        const float step = (2.0f * Pi) / static_cast<float>(segments);
        float prevX = cx + radius;
        float prevY = cy;

        for (int i = 1; i <= segments; ++i) {
            const float a = step * static_cast<float>(i);
            const float x = cx + std::cos(a) * radius;
            const float y = cy + std::sin(a) * radius;
            CE::Renderers::DrawLine(prevX, prevY, x, y, thickness);
            prevX = x;
            prevY = y;
        }
    }

    void ApplyCamera(int& x, int& y, int& size) {
        x = static_cast<int>((x - camera.x) * camera.zoom);
        y = static_cast<int>((y - camera.y) * camera.zoom);
        size = static_cast<int>(size * camera.zoom);
    }

    void Shutdown() {
        switch (CE::Renderers::renderer) {
            case RendererBackend::None:
                return;
            break;

            case RendererBackend::Software:
                #ifdef CE_ENABLE_SOFTWARE_RENDER
                    CE::Renderers::Software::Shutdown();
                #else
                    CE::Log(LogLevel::Error, "[Renderer Manager] Software renderer is not supported");
                    ShowError("Unsupported renderer: Software");
                    std::exit(1);
                #endif
                    break;

            case RendererBackend::OpenGL:
                #ifdef CE_ENABLE_OPENGL_RENDER
                    CE::Renderers::OpenGL::Shutdown();
                #else
                    CE::Log(CE::LogLevel::Error, "[Renderer Manager] OpenGL renderer is not supported");
                    ShowError("Unsupported renderer: OpenGL");
                    std::exit(1);
                #endif
                    break;

                default:
                    return;
                break;
        }
    }

}
