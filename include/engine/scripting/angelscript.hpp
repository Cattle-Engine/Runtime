#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/input/text.hpp"
#include "engine/rendering/resources/model_renderer.hpp"

#include <angelscript.h>

// Forward declare a hella lot of stuff to help compile times
namespace CE {
    class Instance;
    struct GameInfo;

    namespace VFS {
        class VFS;
    }

    namespace Core::GameState {
        class GameStateManager;
    }

    namespace Settings {
        class SettingsManager;
    }

    namespace Renderer {
        class IRenderer;
    }

    namespace Renderer::Resources {
        class TextureManager;
        class ShaderManager;
        class MaterialManager;
        class GPUMeshManager;
        class ModelRenderer;
    } // namespace Renderer::Resources

    namespace Assets::Fonts {
        class FontManager;
    }

    namespace Assets::Model3DImporter {
        class ModelImporter;
    }

    namespace Assets::Animations {
        class AnimatedTextureManager;
    }

    namespace Input {
        class Mouse;
        class Keyboard;
        class TextInput;
        namespace Bindings {
            class BindingManager;
        }
    } // namespace Input

    namespace Audio::Resources {
        class AudioManager;
    }

    namespace Common {
        class Window;
    }

    namespace Common::Containers {
        struct RendererResourcesNameRegistry;
    }

    namespace Scripting::Bindings {
        class ScriptBindings;
    }
} // namespace CE

namespace CE::Scripting {
    class Runtime {
      public:
        Runtime(
            VFS::VFS& vfs, 
            GameInfo& game_info, 
            Settings::SettingsManager& settings_manager, 
            Instance& instance,
            Renderer::IRenderer& renderer, 
            Renderer::Resources::ModelRenderer& model_renderer,
            Renderer::Resources::TextureManager& texture_manager,
            Renderer::Resources::ShaderManager& shader_manager,
            Assets::Fonts::FontManager& font_manager,
            Assets::Model3DImporter::ModelImporter& model_importer,
            Renderer::Resources::GPUMeshManager& gpu_mesh_manager,
            Renderer::Resources::MaterialManager& material_manager,
            Assets::Animations::AnimatedTextureManager& animated_texture_manager, 
            Input::Keyboard& keyboard,
            Input::Mouse& mouse, 
            Input::TextInput& text_input_manager,
            Input::Bindings::BindingManager& binding_manager,
            CE::Common::Containers::RendererResourcesNameRegistry& renderer_resources_name_registry,
            bool output_debug_info, 
            std::string output_debug_as_info_path, 
            Common::Window& window,
            Core::GameState::GameStateManager& game_state_manager,
            Audio::Resources::AudioManager* audio_manager = nullptr
        );

        ~Runtime();

        bool RunStartup();
        // runs the function update() during 2D rendering pass
        bool RunUpdate();
        bool Init();

        const std::string& GetLastError() const;

        int SubscribeStateEvent(
            const std::string& state,
            const std::string& eventName,
            asIScriptFunction* function
        );

        void UnsubscribeStateEvent(int id);

        /**
         * Exposed publicly because all the IScriptBinding impls require access to at-least one of these.
         * Keeping them as public means we don't need to write a ton of trival getter functions
         */
        Runtime& mRuntime = *this;
        CE::Common::Containers::RendererResourcesNameRegistry& mRendererResourcesNameRegistry;
        VFS::VFS& mVFS;
        GameInfo& mGameInfo;
        Settings::SettingsManager& mSettingsManager;
        Instance& mInstance;
        Core::GameState::GameStateManager& mGameStateManager;
        Renderer::IRenderer& mRenderer;
        Renderer::Resources::ModelRenderer& mModelRenderer;
        Renderer::Resources::TextureManager& mTextureManager;
        Renderer::Resources::ShaderManager& mShaderManager;
        Assets::Fonts::FontManager& mFontManager;
        Assets::Model3DImporter::ModelImporter& m3DModelImporter;
        Renderer::Resources::GPUMeshManager& mGPUMeshManager;
        Renderer::Resources::MaterialManager& mMaterialManager;
        Assets::Animations::AnimatedTextureManager& mAnimationManager;
        Input::Keyboard& mKeyboard;
        Input::Mouse& mMouse;
        Input::Bindings::BindingManager& mInputBindingManager;
        Input::TextInput& mTextInput;
        Common::Window& mWindow;
        Audio::Resources::AudioManager* mAudioManager = nullptr;

      private:
        struct ScriptCallbackRegistration {
            std::string state;
            std::string eventName;
            int id = -1;
            asIScriptFunction* function = nullptr;
        };

        bool InvokeStateCallback(asIScriptFunction* callback, const std::string& state, const std::string& eventName);
        void ReleaseStateCallbacks();
        static void MessageCallback(const asSMessageInfo* msg, void* param);
        bool Fail(const std::string& message);

        asIScriptEngine* mScriptEngine = nullptr;
        asIScriptContext* mContext = nullptr;
        asIScriptModule* mScriptModule = nullptr;
        asIScriptFunction* mUpdateFunc = nullptr;
        asIScriptContext* mUpdateCtx = nullptr;
        std::vector<ScriptCallbackRegistration> mStateCallbacks;

        std::unique_ptr<Bindings::ScriptBindings> mScriptBindings;

        std::string mLastError = "";
        std::string OutputDebugASInfoPath = "";
        bool mOutputDebugASInfo = false;
    };
} // namespace CE::Scripting

namespace CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path);
}
