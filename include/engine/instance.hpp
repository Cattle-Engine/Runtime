#pragma once

#include <cstdint>
#include <memory>
#include <SDL3/SDL.h>

#include "engine/assets/fonts.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/assets/textures.hpp"
#include "engine/renderer.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/common/core/event_bus.hpp"
#include "engine/common/core/game_state.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/ui/debug_window.hpp"

// A global to get all instances
inline uint64_t GLOBALINSTANCESCOUNTER;

namespace CE {
    namespace Settings {
        class SettingsManager;
    }
    namespace Scripting {
        class Runtime;
    }

    class Instance {
        public:
            Instance(const char* data_file_path, bool debugmode, 
                Renderer::GPUDeviceHandle& gpudevice);
            int Update();
            bool ShouldExit();
            void Exit();
            float GetDeltaTime() const;
            float GetFrameTime() const;
            int GetFPS() const;
            void ReloadSettings(); // Reload settings, thats all it does :shrug:
            void SetWindowIcon(std::string path);
            int GetInstanceID();
            void SetGameState(const std::string& state);
            const std::string& GetGameState() const;
            CE::Core::EventBus& GetEventBus();
            CE::Core::GameState::GameStateManager& GetGameStateManager();
            ~Instance();
        private:
            void ApplySettingsReload();

            std::unique_ptr<CE::VFS::VFS> gVFS;
            std::unique_ptr<CE::GameInfo> gGameInfo;
            std::unique_ptr<CE::Renderer::IRenderer> gRenderer;
            std::unique_ptr<CE::Assets::Textures::TextureManager> gTextureManager;
            std::unique_ptr<CE::Input::Keyboard> gKeyboardManger;
            std::unique_ptr<CE::Input::Mouse> gMouseManger;
            std::unique_ptr<CE::Assets::Fonts::FontManager> gFontManager;
            std::unique_ptr<CE::Settings::SettingsManager> gSettingsManager;
            std::unique_ptr<CE::Scripting::Runtime> gScriptingManager;
            
            SDL_Window* gWindow = nullptr;
            RendererBackend gRendererBackend = RendererBackend::None;
            bool gDebug = false;
            bool gShouldExit = false;
            bool gWindowFocus = true;
            bool gPendingSettingsReload = false;
            float gDeltaTime = 0.0f;
            float gFrameTime = 0.0f;
            Uint64 gLastFrameCounter = 0;
            Uint64 gPerformanceFrequency = 0;

            // The id for the instance
            int gInstanceID;
            // The id for the window in the instance, provided by SDL
            int gInstanceWindowID;

            CE::Core::EventBus gEventBus;
            CE::Core::GameState::GameStateManager gGameStateManager;
            CE::UI::DebugWindow gDebugWindow;
    };

    using InstanceHandle = std::unique_ptr<Instance>;
}
