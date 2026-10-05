#pragma once

#include <array>
#include <vector>
#include <string>

#include "engine/assets/fonts.hpp"
#include "engine/audio/audio.hpp"
#include "engine/audio/audio_resource_manager.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/settings.hpp"

namespace CE::UI {
    class DebugWindow {
      public:
        DebugWindow(
            CE::Renderer::IRenderer& renderer, 
            CE::Renderer::Resources::TextureManager& texman,
            CE::Renderer::Resources::ShaderManager& shaderman, 
            CE::Assets::Fonts::FontManager& fontman,
            CE::GameInfo& gameinfo, 
            CE::Settings::SettingsManager& settings,
            CE::Audio::Resources::AudioManager* audioman, 
            CE::Input::Keyboard& keyboard,
            CE::Instance& instance, 
            CE::Input::Mouse& mouse,
            CE::Core::Audio::AudioSystem* audio_system
        );

        void Draw();

        void SetOpen(bool open);
        bool IsOpen() const;

      private:
        void DrawInstanceTab();
        void DrawInputTab();
        void DrawSettingsTab();
        void DrawPerformanceTab();
        void DrawRendererTab();
        void DrawAudioTab();

        void UpdateFreeCam(float deltaTime);

        CE::Renderer::IRenderer& mRenderer;
        CE::Renderer::Resources::TextureManager& mTextureManager;
        CE::Renderer::Resources::ShaderManager& mShaderManager;
        CE::Assets::Fonts::FontManager& mFontManager;
        CE::GameInfo& mGameInfo;
        CE::Settings::SettingsManager& mSettings;
        CE::Audio::Resources::AudioManager* mAudioManager;
        CE::Core::Audio::AudioSystem* mAudioSystem = nullptr;
        CE::Input::Keyboard& mKeyboard;
        CE::Instance& mInstance;
        CE::Input::Mouse& mMouse;

        struct SettingsTabState {
            std::array<char, 501> rendererBuffer{};
            bool synced = false;
        };
        std::vector<Core::Audio::AudioDeviceInfo> mAudioDevices; 

        SettingsTabState gSettingsState{};
        std::string mGameState;
        bool mMemoryTrackingEnabled = false;
        std::array<float, 100> gFpsHistory{};
        int gFpsHistoryOffset = 0;
        std::array<char, 64> gAtlasFamilyBuf{};
        int gAtlasSizeBuf = 16;
        bool gOpen = true;
        float mCameraTime = 0.0f;
        bool gFreeCamEnabled = false;
        float gFreeCamSpeed = 5.0f;
        float gFreeCamMouseSensitivity = 0.0025f;

        struct FreeCamState {
            bool enabled = false;
            float speed = 5.0f;
            float sensitivity = 0.02f;
        };

        FreeCamState gFreeCam;
        float gYaw = 0.0f;
        float gPitch = 0.0f;
    };
} // namespace CE::UI
