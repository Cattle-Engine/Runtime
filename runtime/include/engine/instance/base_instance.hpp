#pragma once

#include <memory>
#include <string>

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
#include "engine/ui/logs_window.hpp"
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
#include "engine/settings.hpp"
#include "engine/accs/accs.hpp"

namespace CE {
    extern uint64_t gGlobalInstanceCounter;

    // Plain values the base instance needs from whatever higher-level layer
    // (GameInfo, Settings, ...) the derived class owns. The base never sees
    // those types, only this struct.
    struct InstanceRuntimeConf {
        bool vsync = false;
        int max_fps = 60;

        // window
        std::string window_title;      // already resolved (falls back to game name)
        std::string window_icon_path;
        int window_width = 1280;
        int window_height = 720;
        Common::Window::WindowMode window_mode{};
        bool window_resizable = false;

        // informational (logging only)
        std::string renderer_name;
    };

    // Sleeps out the remainder of a frame so we hit a target FPS.
    class FrameLimiter {
        public:
            void SetMaxFPS(int fps) { mMaxFPS = fps; }
            int GetMaxFPS() const { return mMaxFPS; }

            // Blocks if the frame finished early. Returns the performance counter
            // value at the end of the (possibly delayed) frame.
            Uint64 Throttle(Uint64 frame_start_counter, Uint64 performance_frequency) const;

        private:
            int mMaxFPS = 0; // <= 0 means unlimited
    };

    class BaseInstance {
        public:
            virtual ~BaseInstance();

            BaseInstance(const BaseInstance&) = delete;
            BaseInstance& operator=(const BaseInstance&) = delete;

            // runs 1 frame
            int Update();

            bool ShouldExit() const { return mShouldExit; }
            void Exit() { mShouldExit = true; }
            float GetDeltaTime() const { return mDeltaTime; }
            float GetFrameTime() const { return mFrameTime; }
            int GetFPS() const;
            void ReloadSettings() { mPendingSettingsReload = true; }
            int GetInstanceID() const { return mInstanceID; }
            void SetGameState(const std::string& state);
            const std::string& GetGameState() const;
            Core::EventBus& GetEventBus() { return mEventBus; }
            virtual GameInfo& GetGameInfo() = 0;
            virtual Settings::SettingsManager& GetSettingsManager() = 0;
            Core::GameState::GameStateManager& GetGameStateManager() { return *mGameStateManager; }
            ACCS::ACCS& GetACCS() { return *mACCS; }

        protected:
            BaseInstance() = default; // does NOT initialise, derivied classes call InitialiseBase

            void InitialiseBase(
                bool debug_mode,
                Renderer::GPUDeviceHandle& gpu_device,
                EngineArguements args,
                const std::string& data_path
            );

            // Called after the VFS root is mounted and before the window/renderer exist.
            // Derived classes load whatever they need (game info, settings, extra mounts)
            // and return the values the base needs.
            virtual InstanceRuntimeConf PrepareRuntimeData() = 0;

            // Pushes a new config into the base (vsync, FPS cap). Safe to call at any time
            // after PrepareRuntimeData; derived classes call this from ApplySettingsReload.
            void SetRuntimeConf(const InstanceRuntimeConf& conf);
            const InstanceRuntimeConf& GetRuntimeConf() const { return mConf; }

            // hooks
            // mount the root of the VFS, defaults to a tcf archive mount
            virtual int MountData(const std::string& path);
            // called at the end of BaseInitialise; all base systems exist at this point
            virtual void OnInitialise() {}

            // Return false to shutdown the instance (eg a scripting error)
            virtual bool OnLogic()  { return true; }
            virtual bool OnDraw3D() { return true; }
            virtual bool OnDraw2D() { return true; }
            virtual bool OnImGui()  { return true; }

            void RequestExit() { mShouldExit = true; }
            void SetRenderingEnabled(bool e) { mShouldRender = e; }
            bool IsRenderingEnabled() const { return mShouldRender; }

            std::unique_ptr<Common::FS::VFS::VFS> mVFS;
            std::unique_ptr<Common::Containers::RendererResourcesNameRegistry> mRendererResourcesNameRegistry;
            std::unique_ptr<Renderer::IRenderer> mRenderer;
            std::unique_ptr<Input::Keyboard> mKeyboardManger;
            std::unique_ptr<Input::Mouse> mMouseManger;
            std::unique_ptr<Input::TextInput> mTextInputManager;
            std::unique_ptr<Input::Bindings::BindingManager> mInputBindingManager;
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
            UI::LogWindow mLogsWindow;

            bool mDebug = false;
            bool mShouldExit = false;
            bool mWindowFocus = true;
            bool mShouldRender = true;
            float mDeltaTime = 0.0f;
            float mFrameTime = 0.0f;

            Core::EventBus mEventBus;
            std::unique_ptr<Core::GameState::GameStateManager> mGameStateManager;
            EngineArguements mProgramArguments;
            int mInstanceID = 0;
            int mInstanceWindowID = 0;

            virtual void ApplySettingsReload() {}
            virtual void HandleWindowEvents() {}
        private:
            int Bootstrap_RendererResourceManagers();
            int Bootstrap_InputManagers();
            int Bootstrap_AssetImportersAndManagers();
            int Bootstrap_Video(CE::Renderer::GPUDeviceHandle gpu_device);
            int Bootstrap_Audio();
            int Bootstrap_ACCS();

            Uint64 mLastFrameCounter = 0;
            Uint64 mPerformanceFrequency = 0;
            bool mWasBaseInitDone = false;
            bool mAudioSubsystemInited = false;
            bool mPendingSettingsReload = false;
            RendererBackend mRendererBackend = RendererBackend::None;
            InstanceRuntimeConf mConf;
            FrameLimiter mFrameLimiter;
    };
}