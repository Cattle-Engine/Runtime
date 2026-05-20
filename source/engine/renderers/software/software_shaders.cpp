#include "engine/renderers/software_renderer.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Software {
    void Software_Renderer::LogAboutShaders() {
        CE::Log(LogLevel::Warn, "[Software renderer] Shaders are not supported in software renderer!");
    }

    void Software_Renderer::LoadShader(const char* path) {
        LogAboutShaders();
    }

    void Software_Renderer::UnloadShader(Shader* shader) {
        LogAboutShaders();
    }

    void Software_Renderer::BindShader(Shader* shader) {
        LogAboutShaders();
    }

    void Software_Renderer::UnbindShader() {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderFloat(const char* name, float value) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec2(const char* name, float x, float y) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec3(const char* name, float x, float y, float z) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderVec4(const char* name, float x, float y, float z, float w) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderMat4(const char* name, const float* mat4) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderInt(const char* name, int value) {
        LogAboutShaders();
    }

    void Software_Renderer::SetShaderTexture(const char* name, Texture* texture, int slot) {
        LogAboutShaders();
    }
}