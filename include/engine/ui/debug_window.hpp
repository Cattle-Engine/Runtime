#pragma once

#include <array>

namespace CE {
    class Instance;
    struct GameInfo;

    namespace Renderer {
        class IRenderer;
    }

    namespace Assets {
        namespace Textures {
            class TextureManager;
        }

        namespace Shaders {
            class ShaderManager;
        }

        namespace Skyboxes {
            class SkyBoxManager;
        }

        namespace Fonts {
            class FontManager;
        }

        namespace Audio {
            class AudioManager;
        }
    }

    namespace Settings {
        class SettingsManager;
    }

    namespace Input {
        class Keyboard;
        class Mouse;
    }
}

namespace CE::UI {
    class DebugWindow {
        public:
            void Draw(
                CE::Renderer::IRenderer& renderer,
                CE::Assets::Textures::TextureManager& texman,
                CE::Assets::Shaders::ShaderManager& shaderman,
                CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
                CE::Assets::Fonts::FontManager& fontman,
                CE::GameInfo& gameinfo,
                CE::Settings::SettingsManager& settings,
                CE::Assets::Audio::AudioManager* audioman,
                Input::Keyboard& kbmanger,
                CE::Instance& instance,
                Input::Mouse& msmanager,
                int fps,
                float deltaTime,
                float frameTime
            );

            void SetOpen(bool open);
            bool IsOpen() const;

        private:
            void DrawInstanceTab(CE::GameInfo& gameinfo, CE::Instance& instance);
            void DrawInputTab(CE::Input::Keyboard& kbmanger, CE::Input::Mouse& msmanager);
            void DrawSettingsTab(CE::Settings::SettingsManager& settings, CE::Assets::Audio::AudioManager* audioman);
            void DrawPerformanceTab(
                CE::Renderer::IRenderer& renderer,
                CE::Assets::Textures::TextureManager& texman,
                CE::Assets::Shaders::ShaderManager& shaderman,
                CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
                const CE::Settings::SettingsManager& settings,
                int fps,
                float deltaTime,
                float frameTime
            );
            void DrawRendererTab(
                CE::Renderer::IRenderer& renderer,
                const CE::Settings::SettingsManager& settings,
                CE::Assets::Textures::TextureManager& texman,
                CE::Assets::Shaders::ShaderManager& shaderman,
                CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
                CE::Assets::Fonts::FontManager& fontman
            );
            void DrawAudioTab(CE::Assets::Audio::AudioManager* audioman, CE::Settings::SettingsManager& settings);

            void UpdateFreeCam(
                    CE::Renderer::IRenderer& renderer,
                    Input::Keyboard& keyboard,
                    Input::Mouse& mouse,
                    float deltaTime
            );

            struct SettingsTabState {
                std::array<char, 501> rendererBuffer{};
                bool synced = false;
            };

            struct SkyBoxTabState {
                std::array<char, 256> nameBuffer{};
                std::array<char, 256> frontBuffer{};
                std::array<char, 256> backBuffer{};
                std::array<char, 256> leftBuffer{};
                std::array<char, 256> rightBuffer{};
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
}
