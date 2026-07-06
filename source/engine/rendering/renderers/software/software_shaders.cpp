#include "engine/rendering/renderers/software_renderer.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Software {
    void Software_Renderer::LogAboutShaders() {
        CE_LOG(LogLevel::Warn, "[Software renderer] Shaders are not supported in software renderer!");
    }

    Shader* Software_Renderer::CreateShaderProgram() {
        LogAboutShaders();
        return nullptr;
    }

    Shader* Software_Renderer::LoadShader(const char* path, int fragmentSamplerCount) {
        (void)path;
        (void)fragmentSamplerCount;
        LogAboutShaders();
        return nullptr;
    }

    bool Software_Renderer::LoadShaderStage(Shader* shaderProgram, const char* path, ShaderStage stage, int samplerCount) {
        (void)shaderProgram;
        (void)path;
        (void)stage;
        (void)samplerCount;
        LogAboutShaders();
        return false;
    }

    bool Software_Renderer::UseDefaultShaderStage(Shader* shaderProgram, ShaderStage stage) {
        (void)shaderProgram;
        (void)stage;
        LogAboutShaders();
        return false;
    }

    bool Software_Renderer::CompileShaderProgram(Shader* shaderProgram) {
        (void)shaderProgram;
        LogAboutShaders();
        return false;
    }

    void Software_Renderer::UnloadShader(Shader* shader) {
        (void)shader;
        LogAboutShaders();
    }

    void Software_Renderer::BindShader(Shader* shader) {
        (void)shader;
        LogAboutShaders();
    }

    void Software_Renderer::UnbindShader() {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderFloat(const char* name, float value) {
        (void)name;
        (void)value;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec2(const char* name, float x, float y) {
        (void)name;
        (void)x;
        (void)y;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec3(const char* name, float x, float y, float z) {
        (void)name;
        (void)x;
        (void)y;
        (void)z;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec4(const char* name, float x, float y, float z, float w) {
        (void)name;
        (void)x;
        (void)y;
        (void)z;
        (void)w;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderMat4(const char* name, const float* mat4) {
        (void)name;
        (void)mat4;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderInt(const char* name, int value) {
        (void)name;
        (void)value;
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderTexture(const char* name, Texture* texture, int slot) {
        (void)name;
        (void)texture;
        (void)slot;
        LogAboutShaders();
    }
}
