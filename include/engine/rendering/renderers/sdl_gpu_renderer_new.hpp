#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    class SDLGPURenderer : public IRenderer {
        public:
            void PreWinInit() override {};
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;
        private:
            int Bootstrap_CreateDefault2DPipeline();

            SDL_GPUDevice* mGPUDevice;
            SDL_Window* mWindow;
    };
}