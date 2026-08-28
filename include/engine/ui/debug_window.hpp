#pragma once

#include <array>

#include "engine/assets/fonts.hpp"
#include "engine/rendering/resources/skybox_manager.hpp"
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
        void Draw(CE::Renderer::IRenderer& renderer, CE::Renderer::Resources::TextureManager& texman,
                  CE::Renderer::Resources::ShaderManager& shaderman, CE::Renderer::Resources::SkyBoxManager& skyboxman,
                  CE::Assets::Fonts::FontManager& fontman, CE::GameInfo& gameinfo,
                  CE::Settings::SettingsManager& settings, CE::Audio::Resources::AudioManager* audioman,
                  Input::Keyboard& kbmanger, CE::Instance& instance, Input::Mouse& msmanager, int fps, float deltaTime,
                  float frameTime);

        void SetOpen(bool open);
        bool IsOpen() const;

      private:
        void DrawInstanceTab(CE::GameInfo& gameinfo, CE::Instance& instance);
        void DrawInputTab(CE::Input::Keyboard& kbmanger, CE::Input::Mouse& msmanager);
        void DrawSettingsTab(CE::Settings::SettingsManager& settings, CE::Audio::Resources::AudioManager* audioman);
        void DrawPerformanceTab(CE::Renderer::IRenderer& renderer, CE::Renderer::Resources::TextureManager& texman,
                                CE::Renderer::Resources::ShaderManager& shaderman,
                                CE::Renderer::Resources::SkyBoxManager& skyboxman,
                                const CE::Settings::SettingsManager& settings, int fps, float deltaTime,
                                float frameTime);
        void DrawRendererTab(CE::Renderer::IRenderer& renderer, const CE::Settings::SettingsManager& settings,
                             CE::Renderer::Resources::TextureManager& texman,
                             CE::Renderer::Resources::ShaderManager& shaderman,
                             CE::Renderer::Resources::SkyBoxManager& skyboxman, CE::Assets::Fonts::FontManager& fontman);
        void DrawAudioTab(CE::Audio::Resources::AudioManager* audioman, CE::Settings::SettingsManager& settings);

        void UpdateFreeCam(CE::Renderer::IRenderer& renderer, Input::Keyboard& keyboard, Input::Mouse& mouse,
                           float deltaTime);

        struct SettingsTabState {
            std::array<char, 501> rendererBuffer{};
            bool synced = false;
        };

        struct SkyBoxTabState {
            std::string name;
            std::string front;
            std::string back;
            std::string left;
            std::string right;
            std::string top;
            std::string bottom;
        };

        SettingsTabState gSettingsState{};
        SkyBoxTabState gSkyBoxState{};
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
