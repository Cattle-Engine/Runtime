#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
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

            using Shader = Handle<SDL_GPUShader, SDL_ReleaseGPUShader>;
            using GraphicsPipeline = Handle<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;
            using GPUBuffer = Handle<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
            using GPUTransferBuffer = Handle<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
            using Texture = Handle<SDL_GPUTexture, SDL_ReleaseGPUTexture>;

            struct Vertex {
                float position[3];
                float colour[4];
                float uv[2];
            };

            // Keep the data passed to each mode's vertex shader explicit.  The
            // binding/upload code is intentionally left to the mode flush code.
            struct CameraUniformData {
                glm::mat4 viewProjection{1.0f};
                glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
            };

            struct RenderCommand {
                enum class Type {
                    Draw2D,
                    DrawMesh
                };

                struct MeshDrawCommand {
                    GPUMesh* mesh;
                    glm::mat4 transform;
                };

                struct SpriteDrawCommand {
                    Texture* texture;
                    glm::vec3 position;
                    glm::vec2 size;
                    glm::vec4 colour;
                    float rotation;
                };

                Type type;
                std::variant<MeshDrawCommand, SpriteDrawCommand> data;
            };

            constexpr size_t kMaxQuads = 2500;
            constexpr size_t kMaxVertices = kMaxQuads * 4;
            constexpr size_t kMaxIndices = kMaxQuads * 6;
    }

    class SDLGPURenderer : public IRenderer {
        public:
            SDLGPURenderer(Common::FS::VFS::VFS& vfs) : mVFS(vfs) {}

            void PreWinInit() override {};
            int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) override;
            int Shutdown(SDL_Window* window) override;

            void ChangeCameraPos2D(float x, float y, float zoom) override;
            void ChangeCameraPos3D(const Transform3D& transform) override;
            void SetCamera3D(const Camera3D& camera) override;

            int BeginFrame(SDL_Window* window) override;
            int EndFrame(SDL_Window* window) override;

            void BeginMode2D() override;
            void EndMode2D() override;
            void BeginMode3D() override;
            void EndMode3D() override;
            void SetRenderSize(glm::vec2 size) override;
            void SetClearColour(float r, float g, float b, float a) override;

            // Rendering API stubs.  Keep these here so the new backend is a
            // concrete IRenderer while its draw/shader paths are filled in.
            Texture* LoadTex(const char* path) override;
            Texture* CreateTextureFromData(int width, int height, const void* pixels, TextureFormat format,
                                           int pitch = 0, TextureFilter filter = TextureFilter::Linear,
                                           TextureWrap wrap = TextureWrap::Clamp,
                                           TextureUploadBatch* batch = nullptr) override;
            void DrawTex(Texture* texture, float x, float y, float w, float h, Colour colour, float rotation,
                         TextureFlip flip = TextureFlip::None) override;
            void DrawTexUV(Texture* texture, float x, float y, float w, float h, float u0, float v0, float u1,
                           float v1, Colour colour, float rotation, TextureFlip flip = TextureFlip::None) override;
            void UnloadTex(Texture* texture) override;
            void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint8_t r, uint8_t g,
                              uint8_t b, uint8_t a, float rotation) override;
            void DrawRectLines(float x, float y, float w, float h, float thickness, uint8_t r, uint8_t g,
                               uint8_t b, uint8_t a) override;
            void DrawCircleLines(float cx, float cy, float radius, int segments, float thickness, uint8_t r,
                                 uint8_t g, uint8_t b, uint8_t a) override;

            void DrawRect(
                float x, 
                float y, 
                float w, 
                float h, 
                uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                float rotation
            ) override;
            void DrawCircle(float cx, float cy, float radius, int segments, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t a) override;
            void DrawLine(float x1, float y1, float x2, float y2, float thickness, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a) override;

            Texture* GetErrorTexture() override;
            void* GetNativeTextureHandle(Texture* texture) override;
            int Debug_GetVertCount() override;
            int Debug_GetIndexCount() override;
            int Debug_GetTexIndexCount() override;
            int Debug_GetTexVertCount() override;
            Camera2D* GetCamera() override;
            void SetVSync(bool setting) override;

            CE::Renderer::Shader* CreateShaderProgram() override;

            CE::Renderer::Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) override;
            bool LoadShaderStage(CE::Renderer::Shader* shaderProgram, const char* path, ShaderStage stage,
                                 int samplerCount = 1) override;
            bool UseDefaultShaderStage(CE::Renderer::Shader* shaderProgram, ShaderStage stage) override;
            bool CompileShaderProgram(CE::Renderer::Shader* shaderProgram) override;
            void UnloadShader(CE::Renderer::Shader* shader) override;
            void BindShader(CE::Renderer::Shader* shader) override;
            void UnbindShader() override;
            void SetShaderFloat(const char* name, float value) override;
            void SetShaderVec2(const char* name, float x, float y) override;
            void SetShaderVec3(const char* name, float x, float y, float z) override;
            void SetShaderVec4(const char* name, float x, float y, float z, float w) override;
            void SetShaderMat4(const char* name, const float* mat4) override;
            void SetShaderInt(const char* name, int value) override;
            void SetShaderTexture(const char* name, Texture* texture, int slot) override;

            GPUMesh* CreateGPUMesh(MeshData& mesh) override;
            void DestroyGPUMesh(GPUMesh* mesh) override;
            void DrawMesh(GPUMesh* mesh, Material& material, const Transform3D& transform, bool errorTex) override;
            void DrawMeshMat4(GPUMesh* mesh, Material& material, const glm::mat4& transform,
                              bool errorTex) override;
            void ImGuiStartFrame() override;
            void ImGuiEndFrame(SDL_Window* window) override;
        private:
            using Shader = detail::Shader;
            using GraphicsPipeline = detail::GraphicsPipeline;
            using GPUBuffer = detail::GPUBuffer;
            using GPUTransferBuffer = detail::GPUTransferBuffer;
            using GPUTexture = detail::Texture;

            int Bootstrap_CreateDefault2DPipeline();
            int Bootstrap_CreateDefault3DPipeline();

            Shader LoadShader(const std::string& shader_name, ShaderStage stage,uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_coun, uint32_t storage_texture_count);
            GraphicsPipeline CreateGraphicsPipeline(Shader& vertex, Shader& fragment);
            GPUTexture BasicCreateTextureFromData(
                int width, 
                int height, 
                const void* pixels
            );

            void Draw2DQuad(
                const glm::vec3& position,
                const glm::vec2& size,
                const glm::vec4& colour,
                GPUTexture* texture,
                float rotation
            );

            // Called at the end of EndFrame()
            void Flush2D();

            void Setup2DCamera();
            void Setup3DCamera();
            bool Map2DBatchBuffers();
            void Unmap2DBatchBuffers();
            void ResetFrameState();

            SDL_GPUDevice* mGPUDevice = nullptr;
            SDL_Window* mWindow = nullptr;
            Common::FS::VFS::VFS& mVFS;
            SDL_FColor mClearColour = { 1.0f, 1.0f, 1.0f, 1.0f };

            // buffers and such
            GPUBuffer mVertexBuffer = GPUBuffer();
            GPUBuffer mIndexBuffer = GPUBuffer();
            GPUBuffer m3DMaterialBuffer = GPUBuffer();
            GPUBuffer m3DCameraBuffer = GPUBuffer();

            GPUTransferBuffer mVertexUploadBuffer = GPUTransferBuffer();
            GPUTransferBuffer mIndexUploadBuffer = GPUTransferBuffer();

            // default pipelines and shaders
            Shader mDefault2DVertexShader = Shader();
            Shader mDefault2DFragmentShader = Shader();
            GraphicsPipeline mDefault2DPipeline = GraphicsPipeline();

            Shader mDefault3DVertexShader;
            Shader mDefault3DFragmentShader;
            GraphicsPipeline mDefault3DPipeline;

            // default textures
            GPUTexture mWhiteTexture;
            GPUTexture mErrorTexture;
            GPUTexture mDefaultNormalTexture;

            // per frame stuff
            SDL_GPUCommandBuffer* mCommandBuffer = nullptr;
            SDL_GPUTexture* mSwapchainTexture = nullptr;
            SDL_GPURenderPass* m3DRenderPass = nullptr;
            glm::mat4 m2DMVP{1.0f};
            glm::mat4 m3DMVP{1.0f};
            detail::CameraUniformData m2DCameraUniform{};
            detail::CameraUniformData m3DCameraUniform{};
            detail::Vertex* mMappedVertices = nullptr;
            uint16_t* mMappedIndices = nullptr;
            bool mMode3DActive = false;
            bool mMode2DActive = false;
            size_t mIndexCount = 0;
            size_t mVertexCount = 0;
            std::vector<detail::RenderCommand> mRenderCommands;
            std::shared_ptr<GraphicsPipeline> mCurrentPipeline;

            // 2D rendering 
            Camera2D mCamera2D{};
    };
}
