#include "imgui/imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <SDL3/SDL.h>

#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/events.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    void SDL_GPU_Renderer::ImGuiInit(SDL_Window* window, SDL_GPUDevice* device) {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        IMGUI_CHECKVERSION();
        ImGuiContext* prev_context = ImGui::GetCurrentContext();
        mImguicontext = ImGui::CreateContext();
        ImGui::SetCurrentContext(mImguicontext);

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

        mWindowID = SDL_GetWindowID(window);

        ImGui::SetCurrentContext(prev_context);
    }

    void SDL_GPU_Renderer::ImGuiStartFrame() {
        ImGui::SetCurrentContext(mImguicontext);

        auto indices = CE::Events::GetWindowEventIndices(static_cast<int>(mWindowID));

        for (size_t i : indices)
        {
            const SDL_Event& e = CE::Events::gEvents[i];
            ImGui_ImplSDL3_ProcessEvent(&e);
        }
        ImGui_ImplSDL3_NewFrame();   
        ImGui_ImplSDLGPU3_NewFrame();  
        ImGui::NewFrame();
    }

    void SDL_GPU_Renderer::ImGuiEndFrame(SDL_Window* window) {
        (void)window;
        ImGui::SetCurrentContext(mImguicontext);

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        mPendingImGuiDrawData = (draw_data && draw_data->TotalVtxCount > 0) ? draw_data : nullptr;
    }

    void SDL_GPU_Renderer::ImGuiShutdown() {
        ImGuiContext* prev_context = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(mImguicontext);

        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(mImguicontext);
        mImguicontext = nullptr;
        mPendingImGuiDrawData = nullptr;

        ImGui::SetCurrentContext(prev_context);
    }
}
