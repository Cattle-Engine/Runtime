#include <cstdint>
#include <utility>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace detail {
            template<typename T, void (*Release)(SDL_GPUDevice*, T*)>
            class Handle {
            public:
                Handle() = default;

                explicit Handle(SDL_GPUDevice* device, T* handle)
                    : mDevice(device), mHandle(handle) {}

                ~Handle() {
                    Reset();
                }

                Handle(const Handle&) = delete;
                Handle& operator=(const Handle&) = delete;

                Handle(Handle&& other) noexcept
                    : mDevice(other.mDevice), mHandle(other.mHandle) {
                    other.mDevice = nullptr;
                    other.mHandle = nullptr;
                }

                Handle& operator=(Handle&& other) noexcept {
                    if (this != &other) {
                        Reset();

                        mDevice = other.mDevice;
                        mHandle = other.mHandle;

                        other.mDevice = nullptr;
                        other.mHandle = nullptr;
                    }

                    return *this;
                }

                T* Get() const {
                    return mHandle;
                }

                bool IsValid() const {
                    return mHandle != nullptr;
                }

                void Reset() {
                    if (mDevice && mHandle) {
                        Release(mDevice, mHandle);
                    }

                    mDevice = nullptr;
                    mHandle = nullptr;
                }

            private:
                SDL_GPUDevice* mDevice = nullptr;
                T* mHandle = nullptr;
            };

            using Shader = Handle<SDL_GPUShader, SDL_ReleaseGPUShader>;
            using GraphicsPipeline = Handle<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;
    }

    class SDLGPURenderer : public IRenderer {
        public:
            SDLGPURenderer(Common::FS::VFS::VFS& vfs) : mVFS(vfs) {}

            void PreWinInit() override {};
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;

            Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override;            
        private:
            using Shader = detail::Shader;
            using GraphicsPipeline = detail::GraphicsPipeline;
            struct Vertex {
                float position[3];
                uint8_t color[4];
                float uv[2];
            };

            int Bootstrap_CreateDefault2DPipeline();

            Shader LoadShader(const std::string& shader_name, ShaderStage stage,uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_coun, uint32_t storage_texture_count);
            GraphicsPipeline CreateGraphicsPipeline(Shader& vertex, Shader& fragment);

            SDL_GPUDevice* mGPUDevice;
            SDL_Window* mWindow;
            Common::FS::VFS::VFS& mVFS;

            // default pipelines and shaders
            Shader mDefault2DVertexShader = Shader();
            Shader mDefault2DFragmentShader = Shader();
            GraphicsPipeline mDefault2DPipeline = GraphicsPipeline();
    };
}