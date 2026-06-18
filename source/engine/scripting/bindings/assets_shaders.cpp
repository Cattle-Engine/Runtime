#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

#include <scriptarray/scriptarray.h>

namespace CE::Scripting {
    bool Runtime::RegisterAssetShaderBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Shaders");

        if (mScriptEngine->RegisterEnum("ShaderStage") < 0) {
            return false;
        }

        if (mScriptEngine->RegisterEnumValue("ShaderStage", "Vertex", static_cast<int>(Renderer::ShaderStage::Vertex)) < 0) {
            return false;
        }
        if (mScriptEngine->RegisterEnumValue("ShaderStage", "Fragment", static_cast<int>(Renderer::ShaderStage::Fragment)) < 0) {
            return false;
        }

        CE_REGISTER_GLOBAL("bool CreateShaderProgram(const string &in name)", CreateShaderProgram);
        CE_REGISTER_GLOBAL("bool LoadShader(const string &in path, const string &in name, int fragmentSamplerCount = 4)", LoadShader);
        CE_REGISTER_GLOBAL("bool LoadShaderStage(const string &in program, const string &in path, CE::Graphics::Shaders::ShaderStage stage, int samplerCount = 1)", LoadShaderStage);
        CE_REGISTER_GLOBAL("bool UseDefaultShaderStage(const string &in program, CE::Graphics::Shaders::ShaderStage stage)", UseDefaultShaderStage);
        CE_REGISTER_GLOBAL("bool CompileShaderProgram(const string &in name)", CompileShaderProgram);
        CE_REGISTER_GLOBAL("bool BindShader(const string &in name)", BindShaderProgram);
        CE_REGISTER_GLOBAL("void UnbindShader()", UnbindShaderProgram);
        CE_REGISTER_GLOBAL("void UnloadShader(const string &in name)", UnloadShader);
        CE_REGISTER_GLOBAL("void SetShaderFloat(const string &in uniformName, float value)", SetShaderFloat);
        CE_REGISTER_GLOBAL("void SetShaderVec2(const string &in uniformName, float x, float y)", SetShaderVec2);
        CE_REGISTER_GLOBAL("void SetShaderVec3(const string &in uniformName, float x, float y, float z)", SetShaderVec3);
        CE_REGISTER_GLOBAL("void SetShaderVec4(const string &in uniformName, float x, float y, float z, float w)", SetShaderVec4);
        CE_REGISTER_GLOBAL("void SetShaderMat4(const string &in uniformName, const array<float>@ values)", SetShaderMat4);
        CE_REGISTER_GLOBAL("void SetShaderInt(const string &in uniformName, int value)", SetShaderInt);
        CE_REGISTER_GLOBAL("bool SetShaderTexture(const string &in uniformName, const CE::Texture &in texture, int slot = 0)", SetShaderTexture);

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    bool Runtime::CreateShaderProgram(const std::string& name) {
        return mShaderManager.CreateProgram(name.c_str());
    }

    bool Runtime::LoadShader(const std::string& path, const std::string& name, int fragmentSamplerCount) {
        return mShaderManager.Load(path.c_str(), name.c_str(), fragmentSamplerCount);
    }

    bool Runtime::LoadShaderStage(const std::string& program, const std::string& path, Renderer::ShaderStage stage, int samplerCount) {
        return mShaderManager.LoadStage(program.c_str(), path.c_str(), stage, samplerCount);
    }

    bool Runtime::UseDefaultShaderStage(const std::string& program, Renderer::ShaderStage stage) {
        return mShaderManager.UseDefaultStage(program.c_str(), stage);
    }

    bool Runtime::CompileShaderProgram(const std::string& name) {
        return mShaderManager.Compile(name.c_str());
    }

    bool Runtime::BindShaderProgram(const std::string& name) {
        return mShaderManager.Bind(name.c_str());
    }

    void Runtime::UnbindShaderProgram() {
        mShaderManager.Unbind();
    }

    void Runtime::UnloadShader(const std::string& name) {
        mShaderManager.Unload(name.c_str());
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

        float matrix[16] {};
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
}
