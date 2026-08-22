#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "engine/assets/3d_model_importer.hpp"
#include "engine/ui/debug_window.hpp"
#include "engine/assets/animated_textures.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/assets/skybox_manager.hpp"
#include "engine/audio/audio.hpp"
#include "engine/audio/audio_resource_manager.hpp"
#include "engine/common/core/event_bus.hpp"
#include "engine/common/core/game_state.hpp"
#include "engine/common/containers/registries.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/arguments.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/scripting/angelscript.hpp"

// A global to get all instances
inline uint64_t GLOBALINSTANCESCOUNTER;

namespace CE {
    class Instance {
      public:
        Instance(const char* data_file_path, bool debugmode, Renderer::GPUDeviceHandle& gpudevice,
                 EngineArguements args);
        int Update();
        bool ShouldExit();
        void Exit();
        float GetDeltaTime() const;
        float GetFrameTime() const;
        int GetFPS() const;
        void ReloadSettings(); // Reload settings, that's all it does :shrug:
        void SetWindowIcon(std::string path);
        int GetInstanceID();
        void SetGameState(const std::string& state);
        const std::string& GetGameState() const;
        Core::EventBus& GetEventBus();
        Core::GameState::GameStateManager& GetGameStateManager();
        ~Instance();

      private:
        void ApplySettingsReload();

        // Only to be called at startup!
        int Bootstrap_RendererResourceManagers();
        int Bootstrap_AssetImportersAndManagers();

        std::unique_ptr<VFS::VFS> gVFS;
        std::unique_ptr<GameInfo> gGameInfo;
        std::unique_ptr<Common::Containers::RendererResourcesNameRegistry> mRendererResourcesNameRegistry;
        std::unique_ptr<Settings::SettingsManager> gSettingsManager;
        std::unique_ptr<Renderer::IRenderer> gRenderer;

        std::unique_ptr<Input::Keyboard> gKeyboardManger;
        std::unique_ptr<Input::Mouse> gMouseManger;

        std::unique_ptr<Scripting::Runtime> gScriptingManager;

        std::unique_ptr<Core::Audio::AudioSystem> gAudioSystem;
        std::unique_ptr<Audio::Resources::AudioManager> gAudioManager;

        std::unique_ptr<Renderer::Resources::GPUMeshManager> gGPUMeshManager;
        std::unique_ptr<Renderer::Resources::MaterialManager> gMaterialManager;
        std::unique_ptr<Renderer::Resources::TextureManager> gTextureManager;
        std::unique_ptr<Renderer::Resources::ModelRenderer> gModelRenderer;
        std::unique_ptr<Renderer::Resources::ShaderManager> gShaderManager;

        std::unique_ptr<Assets::Skyboxes::SkyBoxManager> gSkyBoxManager;
        std::unique_ptr<Assets::Animations::AnimatedTextureManager> gAnimatedTextureManager;
        std::unique_ptr<Assets::Model3DImporter::ModelImporter> g3DModelImporter;
        std::unique_ptr<Assets::Fonts::FontManager> gFontManager;

        SDL_Window* gWindow = nullptr;
        RendererBackend gRendererBackend = RendererBackend::None;
        bool gDebug = false;
        bool gShouldExit = false;
        bool gWindowFocus = true;
        bool gShouldRender = true;
        bool gPendingSettingsReload = false;
        float gDeltaTime = 0.0f;
        float gFrameTime = 0.0f;
        Uint64 gLastFrameCounter = 0;
        Uint64 gPerformanceFrequency = 0;

        // The id for the instance
        int gInstanceID;
        // The id for the window in the instance, provided by SDL
        int gInstanceWindowID;

        Core::EventBus gEventBus;
        Core::GameState::GameStateManager gGameStateManager;
        UI::DebugWindow gDebugWindow;
        EngineArguements gProgramArguments;
    };

    using InstanceHandle = std::unique_ptr<Instance>;
} // namespace CE
