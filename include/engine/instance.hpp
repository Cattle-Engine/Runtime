#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "engine/assets/fonts.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/assets/skybox_manager.hpp"
#include "engine/assets/audio.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/assets/3d_model_importer.hpp"
#include "engine/common/misc/arguments.hpp"
#include "engine/assets/animated_textures.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/bootstrap/instance.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/common/core/event_bus.hpp"
#include "engine/common/core/game_state.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/ui/debug_window.hpp"
#include "engine/audio/audio.hpp"

// A global to get all instances
inline uint64_t GLOBALINSTANCESCOUNTER;

namespace CE {
    class Instance {
        public:
            Instance(const char* data_file_path, bool debugmode, 
                Renderer::GPUDeviceHandle& gpudevice, ProgramArguements args);
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
            CE::Core::EventBus& GetEventBus();
            CE::Core::GameState::GameStateManager& GetGameStateManager();
            ~Instance();
        private:
            void ApplySettingsReload();

            // Only to be called at startup!
            int Bootstrap_RendererResourceManagers();
            int Bootstrap_AssetImportersAndManagers();

            std::unique_ptr<CE::VFS::VFS> gVFS;
            std::unique_ptr<CE::GameInfo> gGameInfo;
            std::unique_ptr<CE::Settings::SettingsManager> gSettingsManager;
            std::unique_ptr<CE::Renderer::IRenderer> gRenderer;

            std::unique_ptr<CE::Input::Keyboard> gKeyboardManger;
            std::unique_ptr<CE::Input::Mouse> gMouseManger;

            std::unique_ptr<CE::Scripting::Runtime> gScriptingManager;

            std::unique_ptr<CE::Core::Audio::AudioSystem> gAudioSystem;
            std::unique_ptr<CE::Assets::Audio::AudioManager> gAudioManager;

            std::unique_ptr<CE::Renderer::Resources::GPUMeshManager> gGPUMeshManager;
            std::unique_ptr<CE::Renderer::Resources::MaterialManager> gMaterialManager;
            std::unique_ptr<CE::Renderer::Resources::TextureManager> gTextureManager;
            std::unique_ptr<CE::Renderer::Resources::ModelRenderer> gModelRenderer;
            std::unique_ptr<CE::Renderer::Resources::ShaderManager> gShaderManager;
            
            std::unique_ptr<CE::Assets::Skyboxes::SkyBoxManager> gSkyBoxManager;
            std::unique_ptr<CE::Assets::Animations::AnimatedTextureManager> gAnimatedTextureManager;
            std::unique_ptr<CE::Assets::Model3DImporter::ModelImporter> g3DModelImporter;
            std::unique_ptr<CE::Assets::Fonts::FontManager> gFontManager;
            
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

            CE::Core::EventBus gEventBus;
            CE::Core::GameState::GameStateManager gGameStateManager;
            CE::UI::DebugWindow gDebugWindow;
            ProgramArguements gProgramArguments;

            Renderer::Resources::Model GREMOVEMETESTTHING;
            Renderer::Transform3D GREMOVEMETESTHING_TRANSFORM{};
    };

    using InstanceHandle = std::unique_ptr<Instance>;
}
