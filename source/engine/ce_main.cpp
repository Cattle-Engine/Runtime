#include "engine/renderer.hpp"
#include "engine/state.hpp"
#include "engine/core.hpp"
#include <SDL3/SDL.h>

namespace CE {
    void main() {
        SDL_Event event;
        
        int prevZoom;

        CE::Renderer::Texture* testTex = CE::Renderer::currentRenderer->LoadTex("a.jpg");
        CE::Renderer::Texture* daveTex = CE::Renderer::currentRenderer->LoadTex("tato.webp");

        while (!CE::State::shouldExit) {
            // Poll for events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    CE::State::shouldExit = true;
                }
            }

            CE::Renderer::currentRenderer->BeginFrame(CE::Global::gameWindow);
            
            // Fancy blue
            CE::Renderer::currentRenderer->SetClearColor(0.2f, 0.3f, 0.8f, 1.0f); 

            // A filled rect
            CE::Renderer::currentRenderer->DrawRect(100, 100, 200, 50, 255, 80, 80, 255);

            // Rect outline
            CE::Renderer::currentRenderer->DrawRectLines(100, 100, 200, 50, 2.0f, 255, 255, 255, 255);

            // A line
            CE::Renderer::currentRenderer->DrawLine(0, 0, 400, 400, 1.5f, 0, 255, 0, 255);

            // A filled triangle
            CE::Renderer::currentRenderer->DrawTriangle(
                400, 100,
                500, 300,
                300, 300,
                80, 80, 255, 255
            );

            // A filled circle
            CE::Renderer::currentRenderer->DrawCircle(640, 360, 80.0f, 32, 255, 200, 0, 255);

            // Circle outline
            CE::Renderer::currentRenderer->DrawCircleLines(640, 360, 80.0f, 32, 2.0f, 255, 255, 255, 255);

            CE::Renderer::currentRenderer->DrawTex(testTex, 300, 300, 64, 64, { 255, 255, 255, 255 });
            CE::Renderer::currentRenderer->DrawTex(daveTex, 0, 0, 100, 100, { 255, 255, 255, 255 });

            CE::Renderer::currentRenderer->EndFrame(CE::Global::gameWindow);
        }
    }
}
