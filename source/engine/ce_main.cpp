#include "engine/renderer.hpp"
#include "engine/assets/textures.hpp"
#include "engine/core.hpp"
#include "engine/state.hpp"
#include "engine/common/shutdown.hpp"
#include "engine/core.hpp"
#include <SDL3/SDL.h>

namespace CE {
    void main() {
        SDL_Event event;

        // CE::Renderer::Texture* testTex = CE::Renderer::currentRenderer->LoadTex("a.jpg");
        CE::Renderer::Texture* daveTex = CE::Renderer::currentRenderer->LoadTex("tato.webp");
        CE::Assets::Textures::TextureManager texman(
            CE::Renderer::currentRenderer,
            &CE::Global::GetVFS()
        );

        texman.Load("a.jpg", "testTex");
        while (!CE::State::shouldExit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    CE::State::shouldExit = true;
                }
            }

            CE::Renderer::currentRenderer->BeginFrame(CE::Global::gameWindow);

            CE::Renderer::currentRenderer->SetClearColor(255.0f, 255.0f, 255.0f, 255.0f); 

            CE::Renderer::currentRenderer->DrawRect(100, 100, 200, 50, 255, 80, 80, 255);
            CE::Renderer::currentRenderer->DrawLine(0, 0, 400, 400, 1.5f, 0, 255, 0, 255);
            CE::Renderer::currentRenderer->DrawTriangle(
                400, 100,
                500, 300,
                300, 300,
                80, 80, 255, 255
            );
            CE::Renderer::currentRenderer->DrawCircle(640, 360, 80.0f, 32, 255, 200, 0, 255);
            // CE::Renderer::currentRenderer->DrawTex(testTex, 300, 300, 64, 64, { 255, 255, 255, 255 });
            texman.Draw("testTex", 300, 300, 64, 64, { 255, 255, 255, 255 });
            CE::Renderer::currentRenderer->DrawTex(daveTex, 0, 0, 100, 100, { 0, 255, 75, 255 });

            CE::Renderer::currentRenderer->EndFrame(CE::Global::gameWindow);
        }

        CE::Shutdown::NormalShutdown();
    }
}
