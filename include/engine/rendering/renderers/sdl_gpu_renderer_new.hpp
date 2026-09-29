#include <cstdint>
#include <utility>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <glm/glm.hpp>

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

            struct Vertex {
                float position[3];
                float colour[4];
                float uv[2];
            };

            using Shader = Handle<SDL_GPUShader, SDL_ReleaseGPUShader>;
            using GraphicsPipeline = Handle<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;
            using GPUBuffer = Handle<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
            using GPUTransferBuffer = Handle<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
            using Texture = Handle<SDL_GPUTexture, SDL_ReleaseGPUTexture>;

            constexpr size_t kMaxQuads = 2500;
            constexpr size_t kMaxVertices = kMaxQuads * 4;
            constexpr size_t kMaxIndices = kMaxQuads * 6;
    }

    class SDLGPURenderer : public IRenderer {
        public:
            SDLGPURenderer(Common::FS::VFS::VFS& vfs) : mVFS(vfs) {}

            void PreWinInit() override {};
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;

            int BeginFrame(SDL_Window* window) override;
            int EndFrame(SDL_Window* window) override;

            // TODO: Impliment this
            Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override;            
        private:
            using Shader = detail::Shader;
            using GraphicsPipeline = detail::GraphicsPipeline;
            using GPUBuffer = detail::GPUBuffer;
            using GPUTransferBuffer = detail::GPUTransferBuffer;
            using Texture = detail::Texture;

            int Bootstrap_CreateDefault2DPipeline();

            Shader LoadShader(const std::string& shader_name, ShaderStage stage,uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_coun, uint32_t storage_texture_count);
            GraphicsPipeline CreateGraphicsPipeline(Shader& vertex, Shader& fragment);
            Texture BasicCreateTextureFromData(
                int width, 
                int height, 
                const void* pixels
            );

            SDL_GPUDevice* mGPUDevice;
            SDL_Window* mWindow;
            Common::FS::VFS::VFS& mVFS;

            // buffers and such
            GPUBuffer mVertexBuffer = GPUBuffer();
            GPUBuffer mIndexBuffer = GPUBuffer();

            GPUTransferBuffer mVertexUploadBuffer = GPUTransferBuffer();
            GPUTransferBuffer mIndexUploadBuffer = GPUTransferBuffer();

            // default pipelines and shaders
            Shader mDefault2DVertexShader = Shader();
            Shader mDefault2DFragmentShader = Shader();
            GraphicsPipeline mDefault2DPipeline = GraphicsPipeline();

            // default textures
            Texture mWhiteTexture;
            Texture mErrorTexture;
            Texture mDefaultNormalTexture;

            // per frame stuff
            SDL_GPUCommandBuffer* mCommandBuffer = nullptr;
            SDL_GPUTexture* mSwapchainTexture = nullptr;
            glm::mat4 m2DMVP{};
            bool mMode3DActive = false;
            bool mMode2DActive = false;

            // 2D rendering 
            Camera2D mCamera2D{};
    };
}