#include <format>
#include <memory>
#include <string>

#include "engine/audio/audio.hpp"
#include "engine/common/core/game_state.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/input/input_binder.hpp"
#include "engine/input/text.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/debugger.hpp"
#include "engine/scripting/private/exceptions.hpp"
#include "engine/scripting/private/modules.hpp"
#include "engine/scripting/bindings/bindings_list.hpp"

#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>

namespace {
    static const char* ToString(asEMsgType type) {
        switch (type) {
        case asMSGTYPE_ERROR:
            return "Error";
        case asMSGTYPE_WARNING:
            return "Warning";
        case asMSGTYPE_INFORMATION:
            return "Information";
        default:
            return "Unknown";
        }
    }
} // namespace

namespace CE::Scripting {
    Runtime::Runtime(
        Common::FS::VFS::VFS& vfs, 
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
        Input::TextInput& text_input,
        Input::Bindings::BindingManager& binding_manager, 
        CE::Common::Containers::RendererResourcesNameRegistry& renderer_resources_name_registry,
        bool output_debug_info, 
        std::string output_debug_as_info_path, 
        Common::Window& window,
        Core::GameState::GameStateManager& game_state_manager,
        Audio::Resources::AudioManager* audio_manager,
        Core::Audio::AudioSystem* audio_system
    )
        : mRendererResourcesNameRegistry(renderer_resources_name_registry),
          mVFS(vfs),
          mGameInfo(game_info),
          mSettingsManager(settings_manager),
          mInstance(instance),
          mGameStateManager(game_state_manager),
          mRenderer(renderer),
          mModelRenderer(model_renderer),
          mTextureManager(texture_manager),
          mShaderManager(shader_manager),
          mFontManager(font_manager),
          m3DModelImporter(model_importer),
          mGPUMeshManager(gpu_mesh_manager),
          mMaterialManager(material_manager),
          mAnimationManager(animated_texture_manager),
          mKeyboard(keyboard),
          mMouse(mouse),
          mInputBindingManager(binding_manager),
          mTextInput(text_input),
          mWindow(window),
          mAudioManager(audio_manager),
          mAudioSystem(audio_system) {
        mOutputDebugASInfo = output_debug_info;
        OutputDebugASInfoPath = output_debug_as_info_path;
        mScriptBindings = std::make_unique<CE::Scripting::Bindings::ScriptBindings>();
    }

    Runtime::~Runtime() {
        ReleaseStateCallbacks();
        if (mUpdateCtx != nullptr) {
            mUpdateCtx->Release();
            mUpdateCtx = nullptr;
        }

        if (mImGuiCtx != nullptr) {
            mImGuiCtx->Release();
            mImGuiCtx = nullptr;
        }

        if (mContext != nullptr) {
            mContext->Release();
            mContext = nullptr;
        }

        if (mScriptEngine != nullptr) {
            mScriptEngine->ShutDownAndRelease();
            mScriptEngine = nullptr;
        }
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

        if (mScriptEngine->SetMessageCallback(asFUNCTION(MessageCallback), this, asCALL_CDECL) != 0) {
            CE_LOG(LogLevel::Error, "[Angelscript] Failed to set message callback");
            return false;
        }

        RegisterStdString(mScriptEngine);
        RegisterScriptArray(mScriptEngine, true /* enable gc support */);
        RegisterStdStringUtils(mScriptEngine);

        if (!mScriptBindings->RegisterAllBindings(*mScriptEngine, *this)) {
            return false;
        }

        mContext = mScriptEngine->CreateContext();
        if (mContext == nullptr) {
            return Fail("Failed to create AngelScript script context");
        }

        // enables character literals ('a' is a character literal)
        mScriptEngine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
        // multi-line strings
        mScriptEngine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
        // enable scoped enums likee enum class in C++
        mScriptEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, true);

        CE_LOG(LogLevel::Info, "[AngelScript] Runtime initialised");
        return true;
    }

    bool Runtime::RunStartup() {
        mLastError.clear();
        mCompilerSymbolNames.clear();
        mScriptModule = mScriptEngine->GetModule("main", asGM_ALWAYS_CREATE);
        if (mScriptModule == nullptr) {
            return Fail("Failed to create AngelScript script module");
        }

        std::vector<Impl::GeneratedScriptSection> sections;
        std::string main_entrypoint;
        std::string update_entrypoint;
        std::string imgui_entrypoint;

        try {
            Impl::ModuleImporter importer(mVFS);
            sections = importer.LoadFile(mGameInfo.startupFileName);
            mCompilerSymbolNames = importer.GetDiagnosticSymbolNames();
            main_entrypoint = importer.GetGeneratedEntrypoint("main");
            update_entrypoint = importer.GetGeneratedEntrypoint("update");
            imgui_entrypoint = importer.GetGeneratedEntrypoint("imgui");
            #ifdef CE_DEBUG
                mScriptDebugger = std::make_shared<ScriptDebugger>(importer.GetGeneratedSymbols());
            #endif
        } catch (const Impl::Exceptions::LexerError& error) {
            return Fail(error.what());
        } catch (const Impl::Exceptions::ParserError& error) {
            return Fail(error.what());
        } catch (const Impl::Exceptions::SemanticError& error) {
            return Fail(error.what());
        }

        if (sections.empty()) {
            return Fail(std::format("Failed to load AngelScript startup file '{}'", mGameInfo.startupFileName));
        }

        CE_LOG(LogLevel::Info, "[AngelScript] Loaded startup script '{}'", mGameInfo.startupFileName);
        for (const auto& section : sections) {
            CE_LOG(LogLevel::Debug, "[AngelScript] generated script section '{}':\n\n{}", section.Name,
                   section.Code);
            const int add_result = mScriptModule->AddScriptSection(section.Name.c_str(), section.Code.c_str());
            if (add_result < 0) {
                return Fail(std::format("Failed to add AngelScript script section '{}'", section.Name));
            }
        }

        int r = mScriptModule->Build();
        if (r < 0) {
            // The message callback has already recorded the actionable compiler
            // error. Keep it available to callers instead of replacing it with a
            // generic build failure.
            return mLastError.empty() ? Fail("Failed to build AngelScript module") : false;
        }

        asIScriptFunction* func =
            main_entrypoint.empty() ? nullptr : mScriptModule->GetFunctionByName(main_entrypoint.c_str());

        if (!func) {
            return Fail("AngelScript entrypoint 'void main()' was not found");
        }

        asIScriptContext* ctx = CreateContext();
        if (ctx == nullptr) {
            return Fail("Failed to create AngelScript startup context");
        }

        ctx->Prepare(func);

        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED) {
            ctx->Release();
            return Fail(std::format("AngelScript main() execution failed with code {}", r));
        }

        ctx->Release();
        mUpdateFunc = update_entrypoint.empty() ? nullptr : mScriptModule->GetFunctionByName(update_entrypoint.c_str());
        if (mUpdateFunc == nullptr) {
            CE_LOG(LogLevel::Warn, "[AngelScript] No 'void update()' function found");
        } else {
            mUpdateCtx = CreateContext();
            if (mUpdateCtx == nullptr) {
                return Fail("Failed to create AngelScript update context");
            }
        }

        mImGuiFunc = imgui_entrypoint.empty() ? nullptr : mScriptModule->GetFunctionByName(imgui_entrypoint.c_str());
        if (mImGuiFunc != nullptr) {
            mImGuiCtx = CreateContext();
            if (mImGuiCtx == nullptr) {
                return Fail("Failed to create AngelScript imgui context");
            }
        }

        CE_LOG(LogLevel::Info, "[AngelScript] Startup completed");
        return true;
    }

    bool Runtime::RunUpdate() {
        if (!mUpdateFunc || !mUpdateCtx)
            return true;
        int r = mUpdateCtx->Prepare(mUpdateFunc);
        if (r < 0) {
            return Fail(std::format("Failed to prepare AngelScript update() with code {}", r));
        }

        r = mUpdateCtx->Execute();
        if (r != asEXECUTION_FINISHED) {
            return Fail(std::format("AngelScript update() execution failed with code {}", r));
        }

        return true;
    }

    bool Runtime::RunImGui() {
        if (!mImGuiFunc || !mImGuiCtx)
            return true;

        const int prepare_result = mImGuiCtx->Prepare(mImGuiFunc);
        if (prepare_result < 0) {
            return Fail(std::format("Failed to prepare AngelScript imgui() with code {}", prepare_result));
        }

        const int execute_result = mImGuiCtx->Execute();
        if (execute_result != asEXECUTION_FINISHED) {
            return Fail(std::format("AngelScript imgui() execution failed with code {}", execute_result));
        }

        return true;
    }

    const std::string& Runtime::GetLastError() const {
        return mLastError;
    }

    void Runtime::MessageCallback(const asSMessageInfo* msg, void* param) {
        auto* runtime = static_cast<Runtime*>(param);
        if (msg == nullptr || runtime == nullptr) {
            return;
        }

        std::string diagnostic = msg->message ? msg->message : "";
        for (const auto& [internal_name, source_name] : runtime->mCompilerSymbolNames) {
            size_t position = 0;
            while ((position = diagnostic.find(internal_name, position)) != std::string::npos) {
                diagnostic.replace(position, internal_name.size(), source_name);
                position += source_name.size();
            }
        }

        const std::string message =
            std::format("[AngelScript] {}:{}:{} {}: {}", msg->section ? msg->section : "<unknown>", msg->row, msg->col,
                        ToString(msg->type), diagnostic);

        switch (msg->type) {
        case asMSGTYPE_ERROR:
            runtime->mLastError = message;
            CE_LOG(LogLevel::Error, "{}", message);
            break;
        case asMSGTYPE_WARNING:
            CE_LOG(LogLevel::Warn, "{}", message);
            break;
        case asMSGTYPE_INFORMATION:
        default:
            CE_LOG(LogLevel::Info, "{}", message);
            break;
        }
    }
} // namespace CE::Scripting
