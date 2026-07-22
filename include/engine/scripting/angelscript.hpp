#pragma once

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
    }
    
    namespace Assets::Skyboxes {
        class SkyBoxManager;
    }
    
    namespace Assets::Fonts {
        class FontManager;
    }
    
    namespace Assets::Animations {
        class AnimatedTextureManager;
    }
    
    namespace CE::Assets::Audio {
        
    }
}

namespace CE::Scripting {
    class Runtime {
        Runtime(VFS::VFS &vfs, CE::GameInfo &gameInfo, Settings::SettingsManager &settingsManager, Instance &instance,
                Renderer::IRenderer &renderer, Renderer::Resources::TextureManager &textureManager,
                Renderer::Resources::ShaderManager &shaderManager, Assets::Skyboxes::SkyBoxManager &skyboxManager,
                Assets::Fonts::FontManager &fontManager, Renderer::Resources::GPUMeshManager &gpuMeshManager,
                Renderer::Resources::MaterialManager &materialManager,
                CE::Assets::Animations::AnimatedTextureManager *AnimatedTextureManager, Input::Keyboard &keyboard,
                Input::Mouse &mouse, bool output_debug_info,
                CE::Common::Containers::RendererResourcesNameRegistry &RendererResourcesNameRegistry,
                CE::Assets::Audio::AudioManager *audioManager = nullptr);
    };
}