#include <exception>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_oldnames.h>
#include "ce_runtime/common/misc/arguments.hpp"
#include "ce_runtime/rendering/renderer.hpp"
#include "ce_runtime/common/sdl_events.hpp"

#include "ce_editor/editor.hpp"

bool running = true;

int main(const int argc, const char** argv) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    auto gpu_device = CE::Renderer::CreateGPUDevice(CE::RendererBackend::Vulkan, true);

    try {
        auto instance = CE::Editor::EditorInstance(
            false,
            gpu_device,
            CE::EngineArguements{}
        );

        while (running) {
            CE::SDL_Events::Update();

            for (const auto& e : CE::SDL_Events::gEvents) {
                if (e.type == SDL_EVENT_QUIT) {
                    running = false;
                }
            }

            if (instance.ShouldExit()) {
                running = false;
            }

            instance.Update();
        }
    } catch (const std::exception& e) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "CE Editor Error", 
            e.what(), nullptr
        );
    }

    return 0;
}