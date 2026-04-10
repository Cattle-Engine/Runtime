#include "engine/renderer.hpp"
#include "engine/state.hpp"
#include "engine/core.hpp"
#include <SDL3/SDL.h>

namespace CE {
    void main() {
        SDL_Event event;
        
        int prevZoom;

        while (!CE::State::shouldExit) {
            // Poll for events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    CE::State::shouldExit = true;
                }
            }
            
            CE::Renderer::currentRenderer->BeginFrame(CE::Global::gameWindow);
            
            CE::Renderer::currentRenderer->SetClearColor(0.2f, 0.3f, 0.8f, 1.0f); // cornflower blue or whatever
            CE::Renderer::currentRenderer->DrawRect(100, 100, 200, 150, 255, 0, 0, 255);  

            CE::Renderer::currentRenderer->EndFrame(CE::Global::gameWindow);
        }
    }
}
