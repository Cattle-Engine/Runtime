#pragma once

#include <string>
#include <vector>

#include "engine/assets/fonts.hpp"
#include "engine/rendering/resources/skybox_manager.hpp"
#include "engine/common/containers/registries.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/settings.hpp"

#include <angelscript.h>

class CScriptArray;

namespace CE {
    class Instance;
}

namespace CE::Assets::Audio {
    class AudioManager;
}

namespace CE::Assets::Animations {
    class AnimatedTextureManager;
}

namespace CE::Renderer::Resources {
    class GPUMeshManager;
    class MaterialManager;
} // namespace CE::Renderer::Resources

namespace CE::Scripting {
    class ASMeshData {
      public:
        ASMeshData() = default;

        void AddRef() {
            ++ref_count;
        }
        void Release() {
            if (--ref_count == 0) {
                delete this;
            }
        }

        void SetColour(const Renderer::Colour& colour);

        Renderer::MeshData mesh;

      private:
        unsigned int ref_count = 1;
    };

    struct AudioEffectDesc {
        bool enabled = false;
        int type = 0;
        float cutoffHz = 1000.0f;
        float wetMix = 0.35f;
        float feedback = 0.35f;
        float delayMs = 180.0f;
        float depthMs = 8.0f;
        float rateHz = 0.5f;
        float roomSize = 0.6f;
        float damping = 0.4f;
    };

    struct Vec3Desc {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Transform3DDesc {
        Vec3Desc position{0.0f, 0.0f, 0.0f};
        Vec3Desc rotation{0.0f, 0.0f, 0.0f};
        Vec3Desc scale{1.0f, 1.0f, 1.0f};
    };

    struct Camera3DDesc {
        Vec3Desc position{0.0f, 0.0f, 3.0f};
        Vec3Desc rotation{0.0f, 0.0f, 0.0f};
        Vec3Desc target{0.0f, 0.0f, 0.0f};
        Vec3Desc up{0.0f, 1.0f, 0.0f};
        float fov = 1.0471976f;
        float nearClip = 0.05f;
        float farClip = 2048.0f;
        float orthoSize = 10.0f;
        float aspectOverride = 0.0f;
        bool useTarget = false;
        Renderer::Camera3D::ProjectionMode projection = Renderer::Camera3D::ProjectionMode::Perspective;
    };

    struct TextureHandle {
        Renderer::Resources::TextureHandle handle = 0;
    };

    struct MeshHandle {
        ::CE::Renderer::Resources::MeshHandle handle = 0;
    };

    struct MaterialHandle {
        ::CE::Renderer::Resources::MaterialHandle handle = 0;
    };

    struct MaterialDesc {
        Renderer::Colour tint{};
        float roughness = 1.0f;
        float metallic = 0.0f;
    };

    class Runtime {
      public:
        Runtime(VFS::VFS& vfs, CE::GameInfo& gameInfo, Settings::SettingsManager& settingsManager, Instance& instance,
                Renderer::IRenderer& renderer, Renderer::Resources::TextureManager& textureManager,
                Renderer::Resources::ShaderManager& shaderManager, Assets::Skyboxes::SkyBoxManager& skyboxManager,
                Assets::Fonts::FontManager& fontManager, Renderer::Resources::GPUMeshManager& gpuMeshManager,
                Renderer::Resources::MaterialManager& materialManager,
                CE::Assets::Animations::AnimatedTextureManager* AnimatedTextureManager, Input::Keyboard& keyboard,
                Input::Mouse& mouse, bool output_debug_info,
                CE::Common::Containers::RendererResourcesNameRegistry& RendererResourcesNameRegistry,
                CE::Assets::Audio::AudioManager* audioManager = nullptr);
        ~Runtime();

        bool RunStartup();
        bool RunUpdate();
        bool Initialize();
        const std::string& GetLastError() const;

      private:
        static void MessageCallback(const asSMessageInfo* msg, void* param);

        bool RegisterAssetsBindings();
        bool RegisterAssetCoreBindings();
        bool RegisterAssetMeshBindings();
        bool RegisterAssetTextureBindings();
        bool RegisterAssetShaderBindings();
        bool RegisterAssetPrimitiveBindings();
        bool RegisterAssetAnimationBindings();
        bool Register3DBindings();
        bool RegisterInputBindings();
        bool RegisterInstanceBindings();
        bool RegisterCallbackBindings();
        bool RegisterAudioBindings();
        bool RegisterRegistryBindings();
        bool Fail(const std::string& message);

        struct ScriptCallbackRegistration {
            std::string state;
            std::string eventName;
            int id = -1;
            asIScriptFunction* function = nullptr;
        };

        static void ConstructColour(Renderer::Colour* self);
        static void ConstructColourRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a, Renderer::Colour* self);
        static void ConstructVec3(Vec3Desc* self);
        static void ConstructVec3XYZ(float x, float y, float z, Vec3Desc* self);
        static void ConstructTransform3D(Transform3DDesc* self);
        static void ConstructCamera3D(Camera3DDesc* self);
        static void ConstructMaterial(MaterialDesc* self);

        void LoadTexture(const std::string& path, TextureHandle& texture);
        void UnloadTexture(const TextureHandle& texture);
        void DrawTexture(const TextureHandle& texture, int x, int y, bool flipX = false, bool flipY = false,
                         float tileX = 1.0f, float tileY = 1.0f);
        void DrawTextureEx(const TextureHandle& texture, int x, int y, const Renderer::Colour& colour,
                           bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
        void DrawTextureRot(const TextureHandle& texture, int x, int y, float rotation, bool flipX = false,
                            bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
        void DrawTextureRotEx(const TextureHandle& texture, int x, int y, float rotation,
                              const Renderer::Colour& colour, bool flipX = false, bool flipY = false,
                              float tileX = 1.0f, float tileY = 1.0f);
        void DrawTexturePro(const TextureHandle& texture, int x, int y, int w, int h, float rotation,
                            const Renderer::Colour& colour, bool flipX = false, bool flipY = false, float tileX = 1.0f,
                            float tileY = 1.0f);
        ;
        void DrawRectangle(float x, float y, float w, float h, const Renderer::Colour& colour, float rotation);
        void DrawCircle(float x, float y, float radius, int segments, const Renderer::Colour& colour);
        void DrawLine(float x1, float y1, float x2, float y2, float thickness, const Renderer::Colour& colour);
        void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const Renderer::Colour& colour,
                          float rotation);
        void DrawRectangleLines(float x, float y, float w, float h, float thickness, const Renderer::Colour& colour);
        void DrawCircleLines(float x, float y, float radius, int segments, float thickness,
                             const Renderer::Colour& colour);

        Renderer::Resources::ShaderHandle CreateShaderProgram();
        Renderer::Resources::ShaderHandle LoadShader(const std::string& path, int fragmentSamplerCount = 4);
        bool LoadShaderStage(Renderer::Resources::ShaderHandle handle, const std::string& path,
                             Renderer::ShaderStage stage, int samplerCount = 1);
        bool UseDefaultShaderStage(Renderer::Resources::ShaderHandle handle, Renderer::ShaderStage stage);
        bool CompileShaderProgram(Renderer::Resources::ShaderHandle handle);
        bool BindShaderProgram(Renderer::Resources::ShaderHandle handle);
        void UnbindShaderProgram();
        void UnloadShader(Renderer::Resources::ShaderHandle handle);
        void SetShaderFloat(const std::string& uniformName, float value);
        void SetShaderVec2(const std::string& uniformName, float x, float y);
        void SetShaderVec3(const std::string& uniformName, float x, float y, float z);
        void SetShaderVec4(const std::string& uniformName, float x, float y, float z, float w);
        void SetShaderMat4(const std::string& uniformName, const CScriptArray* values);
        void SetShaderInt(const std::string& uniformName, int value);
        bool SetShaderTexture(const std::string& uniformName, const TextureHandle& texture, int slot);

        bool LoadFont(const std::string& path, const std::string& name, int size);
        void UnloadFont(const std::string& name);
        void DrawText(const std::string& text, int x, int y, float size);
        void DrawTextEx(const std::string& text, const std::string& name, int x, int y, float size,
                        const Renderer::Colour& colour);
        void DrawTextCol(const std::string& text, int x, int y, float size, const Renderer::Colour& colour);

        void LoadAnimation(const std::string& path, const std::string& name);
        void UnloadAnimation(const std::string& name);
        uint32_t CreateAnimationInstance(const std::string& name);
        void DeleteAnimationInstance(uint32_t handle);
        void PlayAnimation(uint32_t handle, int x, int y, bool loop, bool autoRender);
        void PlayAnimationRot(uint32_t handle, int x, int y, bool loop, float rotation, bool autoRender);
        void SetAnimationPosition(uint32_t handle, int x, int y, float rotation);
        void SeekAnimation(uint32_t handle, uint32_t frame);
        void SetAnimationDrawMode(uint32_t handle, bool autoRender);
        void SetAnimationLooping(uint32_t handle, bool loop);
        void SetAnimationTint(uint32_t handle, const Renderer::Colour& colour);
        void PauseAnimation(uint32_t handle);
        void StopAnimation(uint32_t handle);
        void DrawAnimationFrame(uint32_t handle);

        bool IsKeyDown(Input::KeyboardKeys key);
        bool IsKeyPressed(Input::KeyboardKeys key);
        bool IsKeyReleased(Input::KeyboardKeys key);

        bool IsMouseButtonDown(Input::MouseButtons button);
        bool IsMouseButtonPressed(Input::MouseButtons button);
        bool IsMouseButtonReleased(Input::MouseButtons button);
        int GetMouseX();
        int GetMouseY();
        int GetMouseDeltaX();
        int GetMouseDeltaY();
        int GetMouseWheelX();
        int GetMouseWheelY();
        void SetCursorVisibility(Input::MouseVisibility visiblity);
        void LockCursor(bool lock);

        void ExitInstance();
        float GetDeltaTime();
        float GetFrameTime();
        int GetFPS();
        int GetInstanceID();
        void ReloadSettings();
        int RegisterStateCallback(const std::string& state, const std::string& eventName, asIScriptFunction* callback);
        void SetGameState(const std::string& state);
        std::string GetGameState() const;
        int GetSettingInt(const std::string& key, const std::string& section, int fallback);
        float GetSettingFloat(const std::string& key, const std::string& section, float fallback);
        bool GetSettingBool(const std::string& key, const std::string& section, bool fallback);
        std::string GetSettingString(const std::string& key, const std::string& section, const std::string& fallback);
        void SetSettingInt(const std::string& key, const std::string& section, int value);
        void SetSettingFloat(const std::string& key, const std::string& section, float value);
        void SetSettingBool(const std::string& key, const std::string& section, bool value);
        void SetSettingString(const std::string& key, const std::string& section, const std::string& value);
        bool InvokeStateCallback(asIScriptFunction* callback, const std::string& state, const std::string& eventName);
        void ReleaseStateCallbacks();

        void LoadSound(const std::string& path, const std::string& name, int type);
        void UnloadSound(const std::string& name);
        uint32_t CreateSoundInstance(const std::string& name);
        void DeleteSoundInstance(uint32_t handle);
        void PlaySound(uint32_t handle);
        void PauseSound(uint32_t handle);
        void ResumeSound(uint32_t handle);
        void StopSound(uint32_t handle);
        void SeekSound(uint32_t handle, float seconds);
        void SetSoundBus(uint32_t handle, const std::string& bus);
        std::string GetSoundBus(uint32_t handle);
        void SetSoundVolume(uint32_t handle, int volume);
        void SetMasterVolume(float volume);
        void SetMusicVolume(float volume);
        void SetSFXVolume(float volume);
        void AddEffect(uint32_t handle, const std::string& name, const AudioEffectDesc& effect);
        void RemoveEffect(uint32_t handle, const std::string& name);
        void ClearEffects(uint32_t handle);
        void SetCamera3D(const Camera3DDesc& camera);
        void GetCamera3D(Camera3DDesc& outCamera) const;
        void SetSunEnabled(bool enabled);
        void SetSunDirection(const Vec3Desc& direction);
        void SetSunPosition(const Vec3Desc& position);
        void SetSunTint(const Vec3Desc& colour);
        void SetSunIntensity(float intensity);
        void SetAmbientLight(const Vec3Desc& colour, float intensity);
        void LoadSkyBox(const std::string& name, const std::string& frontPath, const std::string& backPath,
                        const std::string& leftPath, const std::string& rightPath, const std::string& topPath,
                        const std::string& bottomPath);
        void SetSkyBox(const std::string& name);
        void ClearSkyBox();
        void UnloadSkyBox(const std::string& name);
        void LoadMaterial(const std::string& name, MaterialHandle& handle);

        int DebugLoadedMaterialsCount() const;
        void CreateMeshHandle(ASMeshData* meshData, MeshHandle& meshHandle);
        void ChangeMesh(const MeshHandle& handle, ASMeshData* meshData);
        void DestroyMesh(const MeshHandle& handle);
        bool HasMesh(const MeshHandle& handle) const;
        void DrawMesh(const MeshHandle& handle, const Transform3DDesc& transform,
                      const MaterialHandle& materialHandle = MaterialHandle(), bool errorTexture = false);

        // Material handle based API
        void CreateMaterialHandle(const MaterialDesc& material, const std::string& textureName,
                                  MaterialHandle& materialHandle);
        void DestroyMaterialHandle(const MaterialHandle& handle);
        bool SetMaterialAlbedo(const MaterialHandle& handle, const std::string& textureName);
        bool SetMaterialTint(const MaterialHandle& handle, const Renderer::Colour& colour);
        bool SetMaterialRoughness(const MaterialHandle& handle, float roughness);
        bool SetMaterialMetallic(const MaterialHandle& handle, float metallic);
        bool HasMaterial(const MaterialHandle& handle) const;

        asIScriptEngine* mScriptEngine = nullptr;
        asIScriptContext* mContext = nullptr;
        asIScriptModule* mScriptModule = nullptr;

        bool mOutputDebugInfo;

        asIScriptFunction* mUpdateFunc = nullptr;
        asIScriptContext* mUpdateCtx = nullptr;
        std::vector<ScriptCallbackRegistration> mStateCallbacks;
        std::string mLastError;

        CE::Common::Containers::RendererResourcesNameRegistry& mRendererResourcesNameRegistry;
        VFS::VFS& mVFS;
        CE::GameInfo& mGameInfo;
        Settings::SettingsManager& mSettingsManager;
        Instance& mInstance;
        Renderer::IRenderer& mRenderer;
        Renderer::Resources::TextureManager& mTextureManager;
        Renderer::Resources::ShaderManager& mShaderManager;
        Assets::Skyboxes::SkyBoxManager& mSkyboxManager;
        Assets::Fonts::FontManager& mFontManager;
        Renderer::Resources::GPUMeshManager& mGPUMeshManager;
        Renderer::Resources::MaterialManager& mMaterialManager;
        CE::Assets::Animations::AnimatedTextureManager* mAnimationManager = nullptr;
        Input::Keyboard& mKeyboard;
        Input::Mouse& mMouse;
        CE::Assets::Audio::AudioManager* mAudioManager = nullptr;
    };
} // namespace CE::Scripting

namespace CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path);
}
