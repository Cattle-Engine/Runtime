#include <SDL3/SDL.h>

#include "engine/common/sdl_events.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/assets/default_font.hpp"
#include "engine/common/tracelog.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

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
        io.ConfigErrorRecovery = true;
        io.ConfigErrorRecoveryEnableAssert = false;
        io.ConfigErrorRecoveryEnableDebugLog = true;
        io.ConfigErrorRecoveryEnableTooltip = true;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   

        // TODO: Enable this when SDL_GPU has support for this io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        style.FontScaleDpi = scale;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;                 // OS windows have square corners
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;    // avoid transparent platform windows
        }

        io.Fonts->Clear();

        ImFontConfig font_cfg{};
        font_cfg.FontDataOwnedByAtlas = false;

        io.FontDefault = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(Assets::Default::default_ce_font),
            static_cast<int>(Assets::Default::default_ce_font_len),
            20.5f * scale,
            &font_cfg
        );

        ImGui_ImplSDL3_InitForSDLGPU(window);

        CE_LOG(LogLevel::Debug, "Platform viewport support: {}",
            (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) != 0);

        CE_LOG(LogLevel::Debug, "Renderer viewport support: {}",
            (io.BackendFlags & ImGuiBackendFlags_RendererHasViewports) != 0);

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

        auto indices = CE::SDL_Events::GetWindowEventIndices(static_cast<int>(mWindowID));

        for (const SDL_Event& e : CE::SDL_Events::gEvents) {
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

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void SDL_GPU_Renderer::ImGuiShutdown(SDL_GPUDevice* device) {
        ImGuiContext* prev_context = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(mImguicontext);

        SDL_WaitForGPUIdle(device);
        ImGui::DestroyPlatformWindows();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(mImguicontext);

        mImguicontext = nullptr;
        mPendingImGuiDrawData = nullptr;
        ImGui::SetCurrentContext(prev_context == mImguicontext ? nullptr : prev_context);
    }
} // namespace CE::Renderer::SDL_GPU_Renderer
