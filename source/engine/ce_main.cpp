#pragma once

#include "engine/renderers.hpp"
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