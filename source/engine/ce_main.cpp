#include "engine/renderer.hpp"
#include "engine/state.hpp"

namespace CE {
    void main() {
        while (CE::State::shouldExit) 
        {
            CE::Renderers::BeginFrame();

            

            CE::Renderers::EndFrame();
        }
    }
}
