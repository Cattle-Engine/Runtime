#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "engine/assets/3d_model_importer.hpp"
#include "engine/assets/animated_textures.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/audio/audio.hpp"
#include "engine/audio/audio_resource_manager.hpp"
#include "engine/common/containers/registries.hpp"
#include "engine/common/core/event_bus.hpp"
#include "engine/common/core/game_state.hpp"

#include "engine/common/fs/vfs.hpp"

#include "engine/common/misc/arguments.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/text.hpp"
#include "engine/input/input_binder.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/gpu_mesh_manager.hpp"
#include "engine/rendering/resources/material_manager.hpp"
#include "engine/rendering/resources/model_renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/common/window.hpp"
#include "engine/accs/accs.hpp"
#include "engine/ui/debug_window.hpp"
#include "engine/ui/angelscript_debugger_window.hpp"

// A global to get all instances
inline uint64_t GLOBALINSTANCESCOUNTER;

namespace CE {
    class Instance {
      public:
        Instance(
          const char* data_file_path, 
          bool debugmode, 
          Renderer::GPUDeviceHandle& gpudevice,
          EngineArguements args
        );

        int Update();
        bool ShouldExit();
        void Exit();
        float GetDeltaTime() const;
        float GetFrameTime() const;
        int GetFPS() const;
        void ReloadSettings(); // Reload settings, that's all it does :shrug:
        int GetInstanceID();
        void SetGameState(const std::string& state);
        const std::string& GetGameState() const;
        Core::EventBus& GetEventBus();
        Core::GameState::GameStateManager& GetGameStateManager();
        ~Instance();

      private:
        void ApplySettingsReload();

        // Only to be called at startup!
        int Bootstrap_RendererResourceManagers(); // NOLINT
        int Bootstrap_InputManagers(); // NOLINT
        int Bootstrap_AssetImportersAndManagers(); // NOLINT
        int Bootstrap_Video(CE::Renderer::GPUDeviceHandle gpu_device); // NOLINT
        int Bootstrap_ACCS(); // NOLINT

        std::unique_ptr<Common::FS::VFS::VFS> mVFS;
        std::unique_ptr<GameInfo> mGameInfo;
        std::unique_ptr<Common::Containers::RendererResourcesNameRegistry> mRendererResourcesNameRegistry;
        std::unique_ptr<Settings::SettingsManager> mSettingsManager;
        std::unique_ptr<Renderer::IRenderer> mRenderer;

        std::unique_ptr<Input::Keyboard> mKeyboardManger;
        std::unique_ptr<Input::Mouse> mMouseManger;
        std::unique_ptr<Input::TextInput> mTextInputManager;
        std::unique_ptr<Input::Bindings::BindingManager> mInputBindingManager;

        std::unique_ptr<Scripting::Runtime> mScriptingManager;

        std::unique_ptr<Core::Audio::AudioSystem> mAudioSystem;
        std::unique_ptr<Audio::Resources::AudioManager> mAudioManager;

        std::unique_ptr<Renderer::Resources::GPUMeshManager> mGPUMeshManager;
        std::unique_ptr<Renderer::Resources::MaterialManager> mMaterialManager;
        std::unique_ptr<Renderer::Resources::TextureManager> mTextureManager;
        std::unique_ptr<Renderer::Resources::ModelRenderer> mModelRenderer;
        std::unique_ptr<Renderer::Resources::ShaderManager> mShaderManager;

        std::unique_ptr<Assets::Animations::AnimatedTextureManager> mAnimatedTextureManager;
        std::unique_ptr<Assets::Model3DImporter::ModelImporter> m3DModelImporter;
        std::unique_ptr<Assets::Fonts::FontManager> mFontManager;

        std::unique_ptr<ACCS::ACCS> mACCS;

        std::unique_ptr<Common::Window> mWindow;
        RendererBackend mRendererBackend = RendererBackend::None;
        bool mDebug = false;
        bool mShouldExit = false;
        bool mWindowFocus = true;
        bool mShouldRender = true;
        bool mPendingSettingsReload = false;
        float mDeltaTime = 0.0f;
        float mFrameTime = 0.0f;
        Uint64 mLastFrameCounter = 0;
        Uint64 mPerformanceFrequency = 0;

        // The id for the instance
        int mInstanceID;
        // The id for the window in the instance, provided by SDL
        int mInstanceWindowID;

        Core::EventBus mEventBus;
        Core::GameState::GameStateManager mGameStateManager;
        std::unique_ptr<UI::DebugWindow> mDebugWindow;
        std::unique_ptr<UI::AngelscriptDebuggerWindow> mASDebuggerWindow;
        EngineArguements mProgramArguments;
    };

    using InstanceHandle = std::unique_ptr<Instance>;
} // namespace CE
