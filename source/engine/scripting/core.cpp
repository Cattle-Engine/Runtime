#include <string>
#include <format>

#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>

#include "engine/common/misc/gameinfo.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/scripting/private/exceptions.hpp"
#include "engine/scripting/private/modules.hpp"

namespace {
    static const char *ToString(asEMsgType type) {
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
}

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
        
        // TODO: add the binding registrations here
        
        mContext = mScriptEngine->CreateContext();
        if (mContext == nullptr) {
            return Fail("Failed to create AngelScript script context");
        }
        
        CE_LOG(LogLevel::Info, "[AngelScript] Runtime initialised");
        return true;
    }
    
    bool Runtime::RunStartup() {
        mScriptModule = mScriptEngine->GetModule("main", asGM_ALWAYS_CREATE);
        if (mScriptModule == nullptr) {
            return Fail("Failed to create AngelScript script module");
        }
        
        std::string code;
        std::string main_entrypoint;
        std::string update_entrypoint;
        
        try {
            Impl::ModuleImporter importer(mVFS);
            code = importer.LoadFile(mGameInfo.startupFileName);
            main_entrypoint = importer.GetGeneratedEntrypoint("main");
            update_entrypoint = importer.GetGeneratedEntrypoint("update");
        } catch (const Impl::Exceptions::LexerError& error) {
            return Fail(std::format("[Lexer] Failed to prepare script file: {}", error.what()));
        } catch (const Impl::Exceptions::ParserError& error) {
            return Fail(std::format("[Parser] Failed to prepare script file: {}", error.what()));
        } catch (const Impl::Exceptions::SemanticError& error) {
            return Fail(std::format("[Semantic Analyser] Failed to prepare script file: {}", error.what()));
        }
        
        if (code.empty()) {
            return Fail(std::format("Failed to load AngelScript startup file '{}'", mGameInfo.startupFileName));
        }
        
        CE_LOG(LogLevel::Info, "[AngelScript] Loaded startup script '{}'", mGameInfo.startupFileName);
        CE_LOG(LogLevel::Debug, "[AngelScript] generated monoscript: \n\n{}", code);
        
        int r = mScriptModule->AddScriptSection("startup", code.c_str());
        
        if (r < 0) {
            return Fail("Failed to add AngelScript startup script section");
        }
        
        r = mScriptModule->Build();
        if (r < 0) {
            return Fail("Failed to build AngelScript module");
        }
        
        asIScriptFunction *func =
        main_entrypoint.empty() ? nullptr : mScriptModule->GetFunctionByName(main_entrypoint.c_str());
        
        if (!func) {
            return Fail("AngelScript entrypoint 'void main()' was not found");
        }
        
        asIScriptContext *ctx = mScriptEngine->CreateContext();
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
            return true;
        }
        
        mUpdateCtx = mScriptEngine->CreateContext();
        if (mUpdateCtx == nullptr) {
            return Fail("Failed to create AngelScript update context");
        }
        
        CE_LOG(LogLevel::Info, "[AngelScript] Startup completed");
        return true;
    }
    
    bool Runtime::RunUpdate() {
        if (!mUpdateFunc || !mUpdateCtx) return true;
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
    
    const std::string& Runtime::GetLastError() const {
        return mLastError;
    }
    
    void Runtime::MessageCallback(const asSMessageInfo *msg, void *param) {
        auto *runtime = static_cast<Runtime *>(param);
        if (msg == nullptr || runtime == nullptr) {
            return;
        }
        
        const std::string message =
        std::format("[AngelScript] {}:{}:{} {}: {}", msg->section ? msg->section : "<unknown>", msg->row, msg->col,ToString(msg->type), msg->message ? msg->message : "");
        
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
}