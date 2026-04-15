#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"
#include "engine/renderer.hpp"
#include "engine/instance.hpp"
#include "engine/common/events.hpp"
#include <SDL3/SDL.h>


int main(int argc, char *argv[]) {
    // Check for flags
    for (int I = 1; I < argc; I++) {
        std::string arg = argv[I];

    }
    CE::Log(CE::LogLevel::Info, "Cattle Engine");
    CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Core::engineVersionString);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    CE::Renderer::GPUDeviceHandle gpu_device = CE::Renderer::CreateGPUDevice(CE::RendererBackend::Vulkan, true);

    try {
        CE::Instance instance("data.tcf", true, gpu_device);
        CE::Instance instance2("data.tcf", true, gpu_device);

        bool shouldexit = false;

        while (!shouldexit) {
            CE::Events::Update();
            if(instance.Update() != 0) {
                shouldexit = true;
            }
            if(instance2.Update() != 0) {
                shouldexit = true;
            }
        }

    } catch (std::runtime_error& e) {
        CE::Log(CE::LogLevel::Fatal, "[Startup] Fatal error: {}", e.what());
    }
    CE::Renderer::DestroyGPUDevice(gpu_device);
    return 0;
}