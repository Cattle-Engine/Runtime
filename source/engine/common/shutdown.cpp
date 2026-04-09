#include <SDL3/SDL.h>

#include "engine/core.hpp"
#include "engine/renderer.hpp"
#include "engine/core.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Shutdown {
    void DurBootstrapShutdown() {
        CE::Log(LogLevel::Info, "[Shutdown] Pre-Bootstrap shutdown called!");
        CE::Log(LogLevel::Info, "[Shutdown] Unmounting game data");
        if (!CE::Global::GetVFS().Unmount("/")) {
            CE::Log(LogLevel::Error, "[Shutdown] Error unmounting game data");
        }

        if (CE::Global::gameWindow != nullptr) {
            CE::Log(LogLevel::Info, "[Shutdown] Closing window");
            SDL_DestroyWindow(CE::Global::gameWindow);
        }

        CE::Log(LogLevel::Info, "[Shutdown] Shutting down renderer");
        CE::Renderer::currentRenderer->Shutdown();
        std::exit(1);
    }
    
    void NormalShutdown() {
        CE::Log(LogLevel::Info, "[Shutdown] Unmounting game data");
        if (!CE::Global::GetVFS().Unmount("/")) {
            CE::Log(LogLevel::Error, "[Shutdown] Error unmounting game data");
        }

        if (CE::Global::gameWindow != nullptr) {
            SDL_DestroyWindow(CE::Global::gameWindow);
        }

        CE::Renderer::currentRenderer->Shutdown();
    }
}