#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <variant>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "engine/common/fs/vfs.hpp"

namespace CE {
    enum class RendererBackend {
        Software,
        OpenGL,
        DX12,
        DX11,
        Metal,
        Vulkan,
        None
    };
}

namespace CE::Renderer {
    class IRenderer;

    struct GPUDevice {
        void* device;
        RendererBackend backend;
    };

    struct Texture;

    struct CubeMap {
        std::shared_ptr<Texture> right {};
        std::shared_ptr<Texture> left {};

        std::shared_ptr<Texture> top {};
        std::shared_ptr<Texture> bottom {};

        std::shared_ptr<Texture> front {};
        std::shared_ptr<Texture> back {};
    };

    using GPUDeviceHandle = std::shared_ptr<GPUDevice>;

    inline RendererBackend renderer = RendererBackend::None;
    inline std::string rendererName = "None";

    IRenderer* CreateRenderer(RendererBackend backend, VFS::VFS* vfs);
    GPUDeviceHandle CreateGPUDevice(RendererBackend backend, bool debugvideo);
    void DestroyGPUDevice(GPUDeviceHandle device);

    struct Camera2D {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    struct Camera3D {
        enum class ProjectionMode {
            Perspective,
            Orthographic
        };

        glm::vec3 position { 0.0f, 0.0f, 3.0f };
        glm::vec3 rotation { 0.0f };
        glm::vec3 target { 0.0f, 0.0f, 0.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };
        float fov = glm::radians(60.0f);
        float nearClip = 0.05f;
        float farClip = 2048.0f;
        float orthoSize = 10.0f;
        float aspectOverride = 0.0f;
        bool useTarget = false;
        ProjectionMode projection = ProjectionMode::Perspective;
    };

    enum class TextureFormat {
        RGBA8,
        RGB8,
        R8,
    };

    struct Colour { // BRITISH SPELLING HEEEEEEEEEHHEHEHE
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
    };

    struct Vector2 {
        int x, y;
    };

    struct Vertex3D {
        glm::vec3 position;
        glm::vec3 normal;
        Colour color;
        glm::vec2 uv;
    };

    struct Transform3D {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
    };

    struct MeshData {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        uint32_t vertex_count;
        uint32_t indice_count;
    };

    struct GPUMesh {
        void* handle = nullptr;
        void* vertex_buffer = nullptr;
        void* index_buffer = nullptr;
        uint32_t vertex_count;
        uint32_t indice_count;
    };

    struct Vertex {
        float x, y, z;
        uint8_t r, g, b, a;
        float u, v;
    };

    struct Texture {
        void* handle;
        int width;
        int height;
        TextureFormat format;
        RendererBackend backend;
    };

    struct Shader {
        void* handle;
        RendererBackend backend;
    };

    struct Material {
        Texture* albedo;
        Colour tint;
        float roughness = 1.0f;
        float metallic = 0.0f;
    };

    struct DirectionalLight {
        bool enabled = true;
        glm::vec3 direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
        glm::vec3 colour = glm::vec3(1.0f, 0.98f, 0.92f);
        float intensity = 1.0f;
    };

    struct AmbientLight {
        glm::vec3 colour = glm::vec3(1.0f);
        float intensity = 0.2f;
    };

    struct LightingState {
        DirectionalLight sun {};
        AmbientLight ambient {};
    };

    enum class ShaderStage {
        Vertex,
        Fragment
    };

    enum class TextureFilter {
        Nearest,
        Linear
    };

    enum class TextureWrap {
        Clamp,          // stretch edge pixels
        Repeat,         // tile the texture
        MirroredRepeat  // tile but mirrored each time
    };

    enum class TextureFlip : uint8_t {
        None = 0,
        Horizontal = 1 << 0,
        Vertical = 1 << 1
    };

    enum class CameraMode {
        Mode2D,
        Mode3D
    };

    constexpr TextureFlip operator|(TextureFlip lhs, TextureFlip rhs) {
        return static_cast<TextureFlip>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
        );
    }

    constexpr TextureFlip operator&(TextureFlip lhs, TextureFlip rhs) {
        return static_cast<TextureFlip>(
            static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
        );
    }

    constexpr bool HasTextureFlip(TextureFlip flips, TextureFlip flag) {
        return (flips & flag) != TextureFlip::None;
    }

    class IRenderer {
        public:
            virtual void PreWinInit() = 0;

            virtual int Init(SDL_Window* window, bool debug, GPUDeviceHandle gdevice) = 0;
            virtual int Shutdown(SDL_Window* window) = 0;

            virtual void ChangeCameraPos2D(float X, float Y, float zoom) = 0;
            virtual void ChangeCameraPos3D(const Transform3D& transform) = 0;
            virtual void SetCamera3D(const Camera3D& camera) {
                mCamera3DState = camera;
                ChangeCameraPos3D({
                    camera.position,
                    camera.rotation,
                    glm::vec3(1.0f)
                });
            }
            virtual void SetSkyBox(const CubeMap& cubemap) {
                mSkyBoxState = cubemap;
            }

            virtual void BeginMode2D() = 0;
            virtual void EndMode2D() = 0;
            virtual void BeginMode3D() = 0;
            virtual void EndMode3D() = 0;

            virtual void DrawRect(float x, float y, float w, float h,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                float rotation) = 0;
            virtual void DrawCircle(float cx, float cy, float radius,
                                    int segments,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
            virtual void DrawLine(float x1, float y1, float x2, float y2,
                                float thickness,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
            virtual void SetClearColor(float r, float g, float b, float a) = 0;
            virtual Texture* LoadTex(const char* path) = 0;
            virtual Texture* CreateTextureFromData(
                int width,
                int height,
                const void* pixels,
                TextureFormat format,
                int pitch = 0,
                TextureFilter filter = TextureFilter::Linear,
                TextureWrap wrap = TextureWrap::Clamp
            ) = 0;
            virtual void DrawTex(Texture* texture, float x, float y,
                                float w, float h, Colour colour,
                                float rotation,
                                TextureFlip flip = TextureFlip::None) = 0;
            virtual void DrawTexUV(Texture* tex,
                float x, float y,
                float w, float h,
                float u0, float v0,
                float u1, float v1,
                Colour colour,
                float rotation,
                TextureFlip flip = TextureFlip::None) = 0;
            virtual void UnloadTex(Texture* texture) = 0;
            virtual void DrawTriangle(
                        float x0, float y0,
                        float x1, float y1,
                        float x2, float y2,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                        float rotation) = 0;
            virtual void DrawRectLines(float x, float y, float w, float h,
                                        float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
            virtual void DrawCircleLines(float cx, float cy, float radius,
                                        int segments, float thickness,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
            virtual int BeginFrame(SDL_Window* window) = 0;
            virtual int EndFrame(SDL_Window* window) = 0;

            virtual Texture* GetErrorTexture() = 0;
            virtual void* GetNativeTextureHandle(Texture* texture) = 0;

            virtual int Debug_GetVertCount() = 0;
            virtual int Debug_GetIndexCount() = 0;
            virtual int Debug_GetTexIndexCount() = 0;
            virtual int Debug_GetTexVertCount() = 0;
            virtual Camera2D* GetCamera() = 0;
            virtual Camera3D* GetCamera3D() {
                return &mCamera3DState;
            }

            virtual void SetVSync(bool setting) = 0;

            virtual Shader* CreateShaderProgram() = 0;
            virtual Shader* LoadShader(const char* path, int fragmentSamplerCount = 4) = 0;
            virtual bool LoadShaderStage(Shader* shaderProgram, const char* path, ShaderStage stage, int samplerCount = 1) = 0;
            virtual bool UseDefaultShaderStage(Shader* shaderProgram, ShaderStage stage) = 0;
            virtual bool CompileShaderProgram(Shader* shaderProgram) = 0;
            virtual void UnloadShader(Shader* shader) = 0;
            virtual void BindShader(Shader* shader) = 0;
            virtual void UnbindShader() = 0;
            virtual void SetShaderFloat(const char* name, float value) = 0;
            virtual void SetShaderVec2(const char* name, float x, float y) = 0;
            virtual void SetShaderVec3(const char* name, float x, float y, float z) = 0;
            virtual void SetShaderVec4(const char* name, float x, float y, float z, float w) = 0;
            virtual void SetShaderMat4(const char* name, const float* mat4) = 0;
            virtual void SetShaderInt(const char* name, int value) = 0;
            virtual void SetShaderTexture(const char* name, Texture* texture, int slot) = 0;

            virtual GPUMesh* CreateGPUMesh(MeshData& mesh) = 0;
            virtual void DestroyGPUMesh(GPUMesh* mesh) = 0;
            virtual void DrawMesh(GPUMesh* mesh, Material& material, const Transform3D& transform, bool error_tex) = 0;

            void SetLightingState(const LightingState& lighting) {
                mLightingState = lighting;
            }

            [[nodiscard]] const LightingState& GetLightingState() const {
                return mLightingState;
            }

            void SetSunEnabled(bool enabled) {
                mLightingState.sun.enabled = enabled;
            }

            void SetSunDirection(const glm::vec3& direction) {
                if (glm::length(direction) > 0.0001f) {
                    mLightingState.sun.direction = glm::normalize(direction);
                }
            }

            void SetSunPosition(const glm::vec3& position) {
                if (glm::length(position) > 0.0001f) {
                    mLightingState.sun.direction = glm::normalize(-position);
                }
            }

            void SetSunTint(const glm::vec3& colour) {
                mLightingState.sun.colour = colour;
            }

            void SetSunIntensity(float intensity) {
                mLightingState.sun.intensity = intensity;
            }

            void SetAmbientLight(const glm::vec3& colour, float intensity) {
                mLightingState.ambient.colour = colour;
                mLightingState.ambient.intensity = intensity;
            }

            [[nodiscard]] const Camera3D& GetCamera3DState() const {
                return mCamera3DState;
            }

            [[nodiscard]] const CubeMap& GetSkyBoxState() const {
                return mSkyBoxState;
            }

            void SetCamera3DFov(float fovRadians) {
                mCamera3DState.fov = fovRadians;
                SetCamera3D(mCamera3DState);
            }

            void SetCamera3DClipPlanes(float nearClip, float farClip) {
                mCamera3DState.nearClip = nearClip;
                mCamera3DState.farClip = farClip;
                SetCamera3D(mCamera3DState);
            }

            void SetCamera3DTarget(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
                mCamera3DState.target = target;
                mCamera3DState.up = up;
                mCamera3DState.useTarget = true;
                SetCamera3D(mCamera3DState);
            }

            void SetCamera3DProjection(Camera3D::ProjectionMode projection) {
                mCamera3DState.projection = projection;
                SetCamera3D(mCamera3DState);
            }

            virtual void ImGuiStartFrame() = 0;
            virtual void ImGuiEndFrame(SDL_Window* window) = 0;
            virtual ~IRenderer() = default;
        protected:
            LightingState mLightingState {};
            Camera3D mCamera3DState {};
            CubeMap mSkyBoxState {};
    };
}
