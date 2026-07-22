#include "engine/scripting/angelscript.hpp"
#include "engine/common/tracelog.hpp"
#include <angelscript.h>

namespace CE::Scripting {
    Runtime::Runtime(
        VFS::VFS &vfs, GameInfo &game_info, Settings::SettingsManager &settings_manager, Instance &instance,
        Renderer::IRenderer &renderer, Renderer::Resources::TextureManager &texture_manager,
        Renderer::Resources::ShaderManager &shader_manager, Assets::Skyboxes::SkyBoxManager &skybox_manager,
        Assets::Fonts::FontManager &font_manager, Renderer::Resources::GPUMeshManager &gpu_mesh_manager,
        Renderer::Resources::MaterialManager &material_manager,
        Assets::Animations::AnimatedTextureManager& animated_texture_manager, Input::Keyboard &keyboard,
        Input::Mouse &mouse, CE::Common::Containers::RendererResourcesNameRegistry &renderer_resources_name_registry,
        bool output_debug_info, std::string output_debug_as_info_path,
        Assets::Audio::AudioManager *audio_manager
    ) : mRendererResourcesNameRegistry(renderer_resources_name_registry), mVFS(vfs), mGameInfo(game_info),
        mSettingsManager(settings_manager), mInstance(instance), mRenderer(renderer), 
        mTextureManager(texture_manager), mShaderManager(shader_manager), mSkyboxManager(skybox_manager),
        mFontManager(font_manager), mGPUMeshManager(gpu_mesh_manager), mMaterialManager(material_manager),
        mAnimationManager(animated_texture_manager), mKeyboard(keyboard), mMouse(mouse), mAudioManager(audio_manager)
    {
        mOutputDebugASInfo = output_debug_info;
        OutputDebugASInfoPath = output_debug_as_info_path;
    }

    bool Runtime::Fail(const std::string& message) {
        mLastError = message;
        CE_LOG(LogLevel::Fatal, "[AngelScript] {}", message);
        return false;
    }

    bool Runtime::Init() {
        mScriptEngine = asCreateScriptEngine();
        if (mScriptEngine == nullptr) {
            return Fail("Failed to create AngelScript engine");
        }
        CE_LOG(LogLevel::Info, "[AngelScript] Created AngelScript engine");
        
        
    }
}