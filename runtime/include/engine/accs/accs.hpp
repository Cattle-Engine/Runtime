#include "engine/accs/world.hpp"
#include "engine/accs/system_manager.hpp"

// Forward declare a hella lot of stuff to help compile times
namespace CE {
    class Instance;
    struct GameInfo;

    namespace Common::FS::VFS {
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

    namespace Core::Audio {
        class AudioSystem;
    }

    namespace Common {
        class Window;
    }

    namespace Common::Containers {
        struct RendererResourcesNameRegistry;
    }
} // namespace CE

namespace CE::ACCS {
    struct Dependencies {
        CE::Instance& instance;
        CE::GameInfo& game_info;

        Common::FS::VFS::VFS& vfs;
        CE::Core::GameState::GameStateManager& game_state_manager;
        CE::Settings::SettingsManager& settings_manager;

        CE::Renderer::IRenderer& renderer;

        CE::Renderer::Resources::TextureManager& texture_manager;
        CE::Renderer::Resources::ShaderManager& shader_manager;
        CE::Renderer::Resources::MaterialManager& material_manager;
        CE::Renderer::Resources::GPUMeshManager& gpu_mesh_manager;
        CE::Renderer::Resources::ModelRenderer& model_renderer;

        CE::Assets::Fonts::FontManager& font_manager;
        CE::Assets::Model3DImporter::ModelImporter& model_importer;
        CE::Assets::Animations::AnimatedTextureManager& animated_texture_manager;

        CE::Input::Mouse& mouse;
        CE::Input::Keyboard& keyboard;
        CE::Input::TextInput& text_input;
        CE::Input::Bindings::BindingManager& binding_manager;

        CE::Audio::Resources::AudioManager* audio_manager;
        CE::Core::Audio::AudioSystem* audio_system;

        CE::Common::Window& window;
        CE::Common::Containers::RendererResourcesNameRegistry& renderer_resources_name_registry;

        Dependencies(
            CE::Instance& instance,
            CE::GameInfo& game_info,
            Common::FS::VFS::VFS& vfs,
            CE::Core::GameState::GameStateManager& game_state_manager,
            CE::Settings::SettingsManager& settings_manager,
            CE::Renderer::IRenderer& renderer,
            CE::Renderer::Resources::TextureManager& texture_manager,
            CE::Renderer::Resources::ShaderManager& shader_manager,
            CE::Renderer::Resources::MaterialManager& material_manager,
            CE::Renderer::Resources::GPUMeshManager& gpu_mesh_manager,
            CE::Renderer::Resources::ModelRenderer& model_renderer,
            CE::Assets::Fonts::FontManager& font_manager,
            CE::Assets::Model3DImporter::ModelImporter& model_importer,
            CE::Assets::Animations::AnimatedTextureManager& animated_texture_manager,
            CE::Input::Mouse& mouse,
            CE::Input::Keyboard& keyboard,
            CE::Input::TextInput& text_input,
            CE::Input::Bindings::BindingManager& binding_manager,
            CE::Audio::Resources::AudioManager* audio_manager,
            CE::Core::Audio::AudioSystem* audio_system,
            CE::Common::Window& window,
            CE::Common::Containers::RendererResourcesNameRegistry& renderer_resources_name_registry
        )
            : instance(instance),
              game_info(game_info),
              vfs(vfs),
              game_state_manager(game_state_manager),
              settings_manager(settings_manager),
              renderer(renderer),
              texture_manager(texture_manager),
              shader_manager(shader_manager),
              material_manager(material_manager),
              gpu_mesh_manager(gpu_mesh_manager),
              model_renderer(model_renderer),
              font_manager(font_manager),
              model_importer(model_importer),
              animated_texture_manager(animated_texture_manager),
              mouse(mouse),
              keyboard(keyboard),
              text_input(text_input),
              binding_manager(binding_manager),
              audio_manager(audio_manager),
              audio_system(audio_system),
              window(window),
              renderer_resources_name_registry(renderer_resources_name_registry) {}
    };

    class ACCS {
        public:
            World world;
            SystemManager system_manager;

            // for internal use of systems only!
            Dependencies dependencies;
            explicit ACCS(Dependencies dependencies) : dependencies(dependencies), system_manager(*this) {}
    };
}