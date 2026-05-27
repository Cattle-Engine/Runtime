#pragma once

#include <angelscript.h>
#include <string>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/settings.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/assets/shaders.hpp"
#include "engine/assets/textures.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/rendering/renderer.hpp"

class CScriptArray;

namespace CE {
    class Instance;
}

namespace CE::Assets::Audio {
    class AudioManager;
}

namespace CE::Assets::Animations {
    class AnimationManager;
}

namespace CE::Scripting {
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

    class Runtime {
        public:
            Runtime(
                VFS::VFS& vfs,
                CE::GameInfo& gameInfo,
                Settings::SettingsManager& settingsManager,
                Instance& instance,
                Renderer::IRenderer& renderer,
                Assets::Textures::TextureManager& textureManager,
                Assets::Shaders::ShaderManager& shaderManager,
                Assets::Fonts::FontManager& fontManager,
                CE::Assets::Animations::AnimationManager* animationManager,
                Input::Keyboard& keyboard,
                Input::Mouse& mouse,
                CE::Assets::Audio::AudioManager* audioManager = nullptr
            );
            ~Runtime();

            bool RunStartup();
            bool RunUpdate(); // TODO ADD AN ACTUAL CALLBACK SYSTEM
            bool Initialize();
            const std::string& GetLastError() const;

        private:
            static void MessageCallback(const asSMessageInfo* msg, void* param);

            bool RegisterAssetsBindings();
            bool RegisterInputBindings();
            bool RegisterInstanceBindings();
            bool RegisterCallbackBindings();
            bool RegisterAudioBindings();
            bool Fail(const std::string& message);

            struct ScriptCallbackRegistration {
                std::string state;
                std::string eventName;
                int id = -1;
                asIScriptFunction* function = nullptr;
            };

            static void ConstructColour(Renderer::Colour* self);
            static void ConstructColourRGBA(
                uint8_t r,
                uint8_t g,
                uint8_t b,
                uint8_t a,
                Renderer::Colour* self
            );

            void LoadTexture(const std::string& path, const std::string& name);
            void UnloadTexture(const std::string& name);
            void DrawTexture(const std::string& name, int x, int y, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
            void DrawTextureEx(const std::string& name, int x, int y, const Renderer::Colour& colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
            void DrawTextureRot(const std::string& name, int x, int y, float rotation, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
            void DrawTextureRotEx(const std::string& name, int x, int y, float rotation, const Renderer::Colour& colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
            void DrawTexturePro(const std::string& name, int x, int y, int w,
                int h, float rotation, const Renderer::Colour& colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f);
            void DrawRectangle(float x, float y, float w, float h, const Renderer::Colour& colour, float rotation);
            void DrawCircle(float x, float y, float radius, int segments, const Renderer::Colour& colour);
            void DrawLine(float x1, float y1, float x2, float y2, float thickness, const Renderer::Colour& colour);
            void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const Renderer::Colour& colour, float rotation);
            void DrawRectangleLines(float x, float y, float w, float h, float thickness, const Renderer::Colour& colour);
            void DrawCircleLines(float x, float y, float radius, int segments, float thickness, const Renderer::Colour& colour);

            bool CreateShaderProgram(const std::string& name);
            bool LoadShader(const std::string& path, const std::string& name, int fragmentSamplerCount = 4);
            bool LoadShaderStage(const std::string& program, const std::string& path, Renderer::ShaderStage stage, int samplerCount = 1);
            bool UseDefaultShaderStage(const std::string& program, Renderer::ShaderStage stage);
            bool CompileShaderProgram(const std::string& name);
            bool BindShaderProgram(const std::string& name);
            void UnbindShaderProgram();
            void UnloadShader(const std::string& name);
            void SetShaderFloat(const std::string& uniformName, float value);
            void SetShaderVec2(const std::string& uniformName, float x, float y);
            void SetShaderVec3(const std::string& uniformName, float x, float y, float z);
            void SetShaderVec4(const std::string& uniformName, float x, float y, float z, float w);
            void SetShaderMat4(const std::string& uniformName, const CScriptArray* values);
            void SetShaderInt(const std::string& uniformName, int value);
            bool SetShaderTexture(const std::string& uniformName, const std::string& textureName, int slot);

            bool LoadFont(const std::string& path, const std::string& name, int size);
            void UnloadFont(const std::string& name);
            void DrawText(const std::string& text, int x, int y, float size);
            void DrawTextEx(const std::string& text, const std::string& name, int x, int y, float size, const Renderer::Colour& colour);
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
            void SetCursorVisiblity(Input::MouseVisiblity visiblity);
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

            asIScriptEngine* mScriptEngine = nullptr;
            asIScriptContext* mContext = nullptr;
            asIScriptModule* mScriptModule = nullptr;

            asIScriptFunction* mUpdateFunc = nullptr;
            asIScriptContext*  mUpdateCtx  = nullptr;   
            std::vector<ScriptCallbackRegistration> mStateCallbacks;
            std::string mLastError;

            VFS::VFS& mVFS;
            CE::GameInfo& mGameInfo;
            Settings::SettingsManager& mSettingsManager;
            Instance& mInstance;
            Renderer::IRenderer& mRenderer;
            Assets::Textures::TextureManager& mTextureManager;
            Assets::Shaders::ShaderManager& mShaderManager;
            Assets::Fonts::FontManager& mFontManager;
            CE::Assets::Animations::AnimationManager* mAnimationManager = nullptr;
            Input::Keyboard& mKeyboard;
            Input::Mouse& mMouse;
            CE::Assets::Audio::AudioManager* mAudioManager = nullptr;
    };
}

namespace CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path);
}
