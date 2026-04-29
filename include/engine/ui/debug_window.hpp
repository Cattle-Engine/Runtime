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

        namespace Fonts {
            class FontManager;
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
                CE::Assets::Fonts::FontManager& fontman,
                CE::GameInfo& gameinfo,
                CE::Settings::SettingsManager& settings,
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
            void DrawSettingsTab(CE::Settings::SettingsManager& settings);
            void DrawPerformanceTab(
                CE::Renderer::IRenderer& renderer,
                CE::Assets::Textures::TextureManager& texman,
                const CE::Settings::SettingsManager& settings,
                int fps,
                float deltaTime,
                float frameTime
            );
            void DrawRendererTab(
                CE::Renderer::IRenderer& renderer,
                const CE::Settings::SettingsManager& settings,
                CE::Assets::Textures::TextureManager& texman,
                CE::Assets::Fonts::FontManager& fontman
            );

            struct SettingsTabState {
                std::array<char, 501> rendererBuffer{};
                bool synced = false;
            };

            SettingsTabState gSettingsState{};
            std::array<float, 100> gFpsHistory{};
            int gFpsHistoryOffset = 0;
            std::array<char, 64> gAtlasFamilyBuf{};
            int gAtlasSizeBuf = 16;
            bool gOpen = true;
            float mCameraTime = 0.0f;
    };
}
