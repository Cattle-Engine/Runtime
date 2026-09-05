#pragma once

#include <string>
#include <vector>

#include "engine/common/fs/vfs.hpp"

#include <angelscript.h>

// Forward declare a hella lot of stuff to help compile times
namespace CE {
    class Instance;
    struct GameInfo;

    namespace VFS {
        class VFS;
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
    } // namespace Renderer::Resources

    namespace Assets::Fonts {
        class FontManager;
    }

    namespace Assets::Animations {
        class AnimatedTextureManager;
    }

    namespace Input {
        class Mouse;
        class Keyboard;
    } // namespace Input

    namespace Audio::Resources {
        class AudioManager;
    }

    namespace Common::Containers {
        struct RendererResourcesNameRegistry;
    }
} // namespace CE

namespace CE::Scripting {
    class Runtime {
      public:
        Runtime(VFS::VFS& vfs, GameInfo& game_info, Settings::SettingsManager& settings_manager, Instance& instance,
                Renderer::IRenderer& renderer, Renderer::Resources::TextureManager& texture_manager,
                Renderer::Resources::ShaderManager& shader_manager,
                Assets::Fonts::FontManager& font_manager, Renderer::Resources::GPUMeshManager& gpu_mesh_manager,
                Renderer::Resources::MaterialManager& material_manager,
                Assets::Animations::AnimatedTextureManager& animated_texture_manager, Input::Keyboard& keyboard,
                Input::Mouse& mouse,
                CE::Common::Containers::RendererResourcesNameRegistry& renderer_resources_name_registry,
                bool output_debug_info, std::string output_debug_as_info_path,
                Audio::Resources::AudioManager* audio_manager = nullptr);
        ~Runtime();

        bool RunStartup();
        // runs the function update() during 2D rendering pass
        bool RunUpdate();
        bool Init();

        const std::string& GetLastError() const;

        /**
         * Exposed publicly because all the IScriptBinding impls require access to at-least one of these.
         * Keeping them as public means we don't need to write a ton of trival getter functions
         */
        CE::Common::Containers::RendererResourcesNameRegistry& mRendererResourcesNameRegistry;
        VFS::VFS& mVFS;
        GameInfo& mGameInfo;
        Settings::SettingsManager& mSettingsManager;
        Instance& mInstance;
        Renderer::IRenderer& mRenderer;
        Renderer::Resources::TextureManager& mTextureManager;
        Renderer::Resources::ShaderManager& mShaderManager;
        Assets::Fonts::FontManager& mFontManager;
        Renderer::Resources::GPUMeshManager& mGPUMeshManager;
        Renderer::Resources::MaterialManager& mMaterialManager;
        Assets::Animations::AnimatedTextureManager& mAnimationManager;
        Input::Keyboard& mKeyboard;
        Input::Mouse& mMouse;
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

        std::string mLastError = "";
        std::string OutputDebugASInfoPath = "";
        bool mOutputDebugASInfo = false;
    };
} // namespace CE::Scripting

namespace CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path);
}
