#include <cstdint>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    class SDLGPURenderer : public IRenderer {
        public:
            SDLGPURenderer(Common::FS::VFS::VFS& vfs) : mVFS(vfs) {}

            void PreWinInit() override {};
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;

            Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override;            
        private:
            int Bootstrap_CreateDefault2DPipeline();

            SDL_GPUShader* LoadShader(const std::string& shader_name, ShaderStage stage,uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_count);

            SDL_GPUDevice* mGPUDevice;
            SDL_Window* mWindow;
            Common::FS::VFS::VFS& mVFS;
    };
}