#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

#include <scriptarray/scriptarray.h>

static void ConstructShaderHandle(CE::Renderer::Resources::ShaderHandle* memory) {
    new (memory) CE::Renderer::Resources::ShaderHandle();
}

static void ConstructShaderHandleCopy(const CE::Renderer::Resources::ShaderHandle& other,
                                      CE::Renderer::Resources::ShaderHandle* memory) {
    new (memory) CE::Renderer::Resources::ShaderHandle(other);
}

static void DestructShaderHandle(CE::Renderer::Resources::ShaderHandle* memory) {
    memory->~ShaderHandle();
}

namespace CE::Scripting {
    bool Runtime::RegisterAssetShaderBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE");

        /*
        // Register uint64 if not already registered
        if (mScriptEngine->RegisterTypedef("uint64", "uint64") < 0) {
            // uint64 might already be registered, ignore error
        } */

        // Register the ShaderHandle struct
        int r = mScriptEngine->RegisterObjectType("ShaderHandle", sizeof(Renderer::Resources::ShaderHandle),
                                                  asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
        if (r < 0) {
            return false;
        }

        // Default constructor
        r = mScriptEngine->RegisterObjectBehaviour("ShaderHandle", asBEHAVE_CONSTRUCT, "void f()",
                                                   asFUNCTION(ConstructShaderHandle), asCALL_CDECL_OBJLAST);
        if (r < 0) {
            return false;
        }

        // Copy constructor
        r = mScriptEngine->RegisterObjectBehaviour("ShaderHandle", asBEHAVE_CONSTRUCT, "void f(const ShaderHandle &in)",
                                                   asFUNCTION(ConstructShaderHandleCopy), asCALL_CDECL_OBJLAST);
        if (r < 0) {
            return false;
        }

        // Destructor
        r = mScriptEngine->RegisterObjectBehaviour("ShaderHandle", asBEHAVE_DESTRUCT, "void f()",
                                                   asFUNCTION(DestructShaderHandle), asCALL_CDECL_OBJLAST);
        if (r < 0) {
            return false;
        }

        // Property
        r = mScriptEngine->RegisterObjectProperty("ShaderHandle", "uint64 id",
                                                  asOFFSET(Renderer::Resources::ShaderHandle, id));
        if (r < 0) {
            return false;
        }

        // Boolean conversion operator (opImplCast for implicit cast to bool)
        r = mScriptEngine->RegisterObjectMethod("ShaderHandle", "bool opImplCast() const",
                                                asMETHOD(Renderer::Resources::ShaderHandle, operator bool),
                                                asCALL_THISCALL);
        if (r < 0) {
            return false;
        }

        r = mScriptEngine->RegisterObjectMethod("ShaderHandle", "bool opEquals(const ShaderHandle &in) const",
                                                asMETHODPR(Renderer::Resources::ShaderHandle, operator==,
                                                           (const Renderer::Resources::ShaderHandle&) const, bool),
                                                asCALL_THISCALL);
        if (r < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Shaders");

        r = mScriptEngine->RegisterEnum("ShaderStage");

        r = mScriptEngine->RegisterEnumValue("ShaderStage", "Vertex", static_cast<int>(Renderer::ShaderStage::Vertex));
        if (r < 0) {
            mScriptEngine->SetDefaultNamespace("");
            return false;
        }

        r = mScriptEngine->RegisterEnumValue("ShaderStage", "Fragment",
                                             static_cast<int>(Renderer::ShaderStage::Fragment));
        if (r < 0) {
            mScriptEngine->SetDefaultNamespace("");
            return false;
        }

        CE_REGISTER_GLOBAL("ShaderHandle CreateShaderProgram()", CreateShaderProgram);
        CE_REGISTER_GLOBAL("ShaderHandle LoadShader(const string &in path, int fragmentSamplerCount = 4)", LoadShader);
        CE_REGISTER_GLOBAL("bool LoadShaderStage(ShaderHandle handle, const string &in path, "
                           "CE::Graphics::Shaders::ShaderStage stage, int samplerCount = 1)",
                           LoadShaderStage);
        CE_REGISTER_GLOBAL("bool UseDefaultShaderStage(ShaderHandle handle, CE::Graphics::Shaders::ShaderStage stage)",
                           UseDefaultShaderStage);
        CE_REGISTER_GLOBAL("bool CompileShaderProgram(ShaderHandle handle)", CompileShaderProgram);
        CE_REGISTER_GLOBAL("bool BindShader(ShaderHandle handle)", BindShaderProgram);
        CE_REGISTER_GLOBAL("void UnbindShader()", UnbindShaderProgram);
        CE_REGISTER_GLOBAL("void UnloadShader(ShaderHandle handle)", UnloadShader);
        CE_REGISTER_GLOBAL("void SetShaderFloat(const string &in uniformName, float value)", SetShaderFloat);
        CE_REGISTER_GLOBAL("void SetShaderVec2(const string &in uniformName, float x, float y)", SetShaderVec2);
        CE_REGISTER_GLOBAL("void SetShaderVec3(const string &in uniformName, float x, float y, float z)",
                           SetShaderVec3);
        CE_REGISTER_GLOBAL("void SetShaderVec4(const string &in uniformName, float x, float y, float z, float w)",
                           SetShaderVec4);
        CE_REGISTER_GLOBAL("void SetShaderMat4(const string &in uniformName, const array<float>@ values)",
                           SetShaderMat4);
        CE_REGISTER_GLOBAL("void SetShaderInt(const string &in uniformName, int value)", SetShaderInt);
        CE_REGISTER_GLOBAL(
            "bool SetShaderTexture(const string &in uniformName, const CE::Texture &in texture, int slot = 0)",
            SetShaderTexture);

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    Renderer::Resources::ShaderHandle Runtime::CreateShaderProgram() {
        return mShaderManager.CreateProgram();
    }

    Renderer::Resources::ShaderHandle Runtime::LoadShader(const std::string& path, int fragmentSamplerCount) {
        return mShaderManager.Load(path.c_str(), fragmentSamplerCount);
    }

    bool Runtime::LoadShaderStage(Renderer::Resources::ShaderHandle handle, const std::string& path,
                                  Renderer::ShaderStage stage, int samplerCount) {
        return mShaderManager.LoadStage(handle, path, stage, samplerCount);
    }

    bool Runtime::UseDefaultShaderStage(Renderer::Resources::ShaderHandle handle, Renderer::ShaderStage stage) {
        return mShaderManager.UseDefaultStage(handle, stage);
    }

    bool Runtime::CompileShaderProgram(Renderer::Resources::ShaderHandle handle) {
        return mShaderManager.Compile(handle);
    }

    bool Runtime::BindShaderProgram(Renderer::Resources::ShaderHandle handle) {
        return mShaderManager.Bind(handle);
    }

    void Runtime::UnbindShaderProgram() {
        mShaderManager.Unbind();
    }

    void Runtime::UnloadShader(Renderer::Resources::ShaderHandle handle) {
        mShaderManager.Unload(handle);
    }

    void Runtime::SetShaderFloat(const std::string& uniformName, float value) {
        mShaderManager.SetFloat(uniformName.c_str(), value);
    }

    void Runtime::SetShaderVec2(const std::string& uniformName, float x, float y) {
        mShaderManager.SetVec2(uniformName.c_str(), x, y);
    }

    void Runtime::SetShaderVec3(const std::string& uniformName, float x, float y, float z) {
        mShaderManager.SetVec3(uniformName.c_str(), x, y, z);
    }

    void Runtime::SetShaderVec4(const std::string& uniformName, float x, float y, float z, float w) {
        mShaderManager.SetVec4(uniformName.c_str(), x, y, z, w);
    }

    void Runtime::SetShaderMat4(const std::string& uniformName, const CScriptArray* values) {
        if (!values || values->GetSize() < 16) {
            return;
        }

        float matrix[16]{};
        for (asUINT i = 0; i < 16; ++i) {
            matrix[i] = *static_cast<const float*>(values->At(i));
        }
        mShaderManager.SetMat4(uniformName.c_str(), matrix);
    }

    void Runtime::SetShaderInt(const std::string& uniformName, int value) {
        mShaderManager.SetInt(uniformName.c_str(), value);
    }

    bool Runtime::SetShaderTexture(const std::string& uniformName, const TextureHandle& texture, int slot) {
        return mShaderManager.SetTexture(uniformName.c_str(), texture.handle, slot);
    }
} // namespace CE::Scripting