#include "imgui/imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <SDL3/SDL.h>

#include "engine/renderers/sdl_gpu_renderer.hpp"

namespace CE::Renderer::SDL_GPU_Renderer::ImGuiImpl {
    void ImGuiInit(SDL_Window* window, SDL_GPUDevice* device) {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        style.FontScaleDpi = scale;

        ImGui_ImplSDL3_InitForSDLGPU(window);

        ImGui_ImplSDLGPU3_InitInfo info{};
        info.Device = device;
        info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
        info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

        ImGui_ImplSDLGPU3_Init(&info);
    }
    void ImGuiNewFrame() {
        ImGui_ImplSDL3_NewFrame();    
        ImGui_ImplSDLGPU3_NewFrame(); 
        ImGui::NewFrame();
    }

    void Shutdown() {
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}