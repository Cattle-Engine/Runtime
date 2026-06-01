#include "engine/assets/shaders.hpp"

#include "engine/common/tracelog.hpp"
#include <algorithm>

namespace CE::Assets::Shaders {
    ShaderManager::ShaderManager(
        CE::Renderer::IRenderer* renderer,
        CE::VFS::VFS* vfs,
        CE::Assets::Textures::TextureManager* textureManager
    ) :
        renderer(renderer),
        vfs(vfs),
        textureManager(textureManager) {
    }

    const char* ShaderManager::StageToString(CE::Renderer::ShaderStage stage) const {
        return stage == CE::Renderer::ShaderStage::Vertex ? "vertex" : "fragment";
    }

    ShaderManager::ManagedShader* ShaderManager::FindShader(const char* name) {
        auto it = shaders.find(name ? name : "");
        return it != shaders.end() ? &it->second : nullptr;
    }

    const ShaderManager::ManagedShader* ShaderManager::FindShader(const char* name) const {
        auto it = shaders.find(name ? name : "");
        return it != shaders.end() ? &it->second : nullptr;
    }

    bool ShaderManager::CreateProgram(const char* name) {
        if (!name || name[0] == '\0') {
            CE::Log(LogLevel::Error, "[Shader Manager] CreateProgram called with an empty name");
            return false;
        }

        Unload(name);

        ManagedShader shaderInfo{};
        shaderInfo.Shader = renderer->CreateShaderProgram();
        shaderInfo.IsErrorShader = shaderInfo.Shader == nullptr;
        shaders[name] = shaderInfo;

        if (!shaderInfo.Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Failed to create shader program '{}'", name);
            return false;
        }

        return true;
    }

    bool ShaderManager::Load(const char* filepath, const char* name, int fragmentSamplerCount) {
        if (!name || name[0] == '\0' || !filepath || filepath[0] == '\0') {
            CE::Log(LogLevel::Error, "[Shader Manager] Load called with invalid arguments");
            return false;
        }

        Unload(name);

        ManagedShader shaderInfo{};
        shaderInfo.ProgramPath = filepath;
        shaderInfo.VertexPath = std::string(filepath) + ".vert";
        shaderInfo.FragmentPath = std::string(filepath) + ".frag";
        shaderInfo.Shader = renderer->LoadShader(filepath, fragmentSamplerCount);
        shaderInfo.IsCompiled = shaderInfo.Shader != nullptr;
        shaderInfo.IsErrorShader = shaderInfo.Shader == nullptr;
        shaderInfo.UsesDefaultVertex = false;
        shaderInfo.UsesDefaultFragment = false;
        shaderInfo.FragmentSamplerCount = std::max(1, fragmentSamplerCount);
        shaders[name] = shaderInfo;

        if (!shaderInfo.Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Failed to load shader program '{}' from '{}'", name, filepath);
            return false;
        }

        return true;
    }

    bool ShaderManager::LoadStage(const char* name, const char* filepath, CE::Renderer::ShaderStage stage, int samplerCount) {
        ManagedShader* shaderInfo = FindShader(name);
        if (!shaderInfo) {
            if (!CreateProgram(name)) {
                return false;
            }
            shaderInfo = FindShader(name);
        }

        if (!shaderInfo || !shaderInfo->Shader) {
            return false;
        }

        if (!filepath || filepath[0] == '\0') {
            CE::Log(LogLevel::Error, "[Shader Manager] LoadStage called with an empty path for {}", StageToString(stage));
            return false;
        }

        if (!renderer->LoadShaderStage(shaderInfo->Shader, filepath, stage, samplerCount)) {
            shaderInfo->IsErrorShader = true;
            return false;
        }

        shaderInfo->IsErrorShader = false;
        shaderInfo->IsCompiled = false;
        if (stage == CE::Renderer::ShaderStage::Vertex) {
            shaderInfo->VertexPath = filepath;
            shaderInfo->UsesDefaultVertex = false;
        } else {
            shaderInfo->FragmentPath = filepath;
            shaderInfo->UsesDefaultFragment = false;
            shaderInfo->FragmentSamplerCount = std::max(1, samplerCount);
        }
        return true;
    }

    bool ShaderManager::UseDefaultStage(const char* name, CE::Renderer::ShaderStage stage) {
        ManagedShader* shaderInfo = FindShader(name);
        if (!shaderInfo || !shaderInfo->Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] UseDefaultStage called for missing shader '{}'", name ? name : "");
            return false;
        }

        if (!renderer->UseDefaultShaderStage(shaderInfo->Shader, stage)) {
            return false;
        }

        shaderInfo->IsCompiled = false;
        if (stage == CE::Renderer::ShaderStage::Vertex) {
            shaderInfo->UsesDefaultVertex = true;
            shaderInfo->VertexPath.clear();
        } else {
            shaderInfo->UsesDefaultFragment = true;
            shaderInfo->FragmentPath.clear();
        }
        return true;
    }

    bool ShaderManager::Compile(const char* name) {
        ManagedShader* shaderInfo = FindShader(name);
        if (!shaderInfo || !shaderInfo->Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Compile called for missing shader '{}'", name ? name : "");
            return false;
        }

        shaderInfo->IsCompiled = renderer->CompileShaderProgram(shaderInfo->Shader);
        shaderInfo->IsErrorShader = !shaderInfo->IsCompiled;
        return shaderInfo->IsCompiled;
    }

    bool ShaderManager::Bind(const char* name) {
        ManagedShader* shaderInfo = FindShader(name);
        if (!shaderInfo || !shaderInfo->Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Bind called for missing shader '{}'", name ? name : "");
            return false;
        }

        if (!shaderInfo->IsCompiled && !Compile(name)) {
            return false;
        }

        renderer->BindShader(shaderInfo->Shader);
        gBoundShaderName = name;
        return true;
    }

    void ShaderManager::Unbind() {
        renderer->UnbindShader();
        gBoundShaderName.clear();
    }

    void ShaderManager::Unload(const char* name) {
        auto it = shaders.find(name ? name : "");
        if (it == shaders.end()) {
            return;
        }

        if (!gBoundShaderName.empty() && gBoundShaderName == it->first) {
            Unbind();
        }

        if (it->second.Shader) {
            renderer->UnloadShader(it->second.Shader);
            it->second.Shader = nullptr;
        }

        shaders.erase(it);
    }

    void ShaderManager::UnloadAll() {
        Unbind();
        for (auto& [name, shaderInfo] : shaders) {
            if (shaderInfo.Shader) {
                renderer->UnloadShader(shaderInfo.Shader);
                shaderInfo.Shader = nullptr;
            }
        }
        shaders.clear();
    }

    void ShaderManager::SetFloat(const char* uniformName, float value) {
        renderer->SetShaderFloat(uniformName, value);
    }

    void ShaderManager::SetVec2(const char* uniformName, float x, float y) {
        renderer->SetShaderVec2(uniformName, x, y);
    }

    void ShaderManager::SetVec3(const char* uniformName, float x, float y, float z) {
        renderer->SetShaderVec3(uniformName, x, y, z);
    }

    void ShaderManager::SetVec4(const char* uniformName, float x, float y, float z, float w) {
        renderer->SetShaderVec4(uniformName, x, y, z, w);
    }

    void ShaderManager::SetMat4(const char* uniformName, const float* value) {
        renderer->SetShaderMat4(uniformName, value);
    }

    void ShaderManager::SetInt(const char* uniformName, int value) {
        renderer->SetShaderInt(uniformName, value);
    }

    bool ShaderManager::SetTexture(const char* uniformName, const char* textureName, int slot) {
        if (!textureManager) {
            CE::Log(LogLevel::Error, "[Shader Manager] No texture manager available for SetTexture");
            return false;
        }

        auto texture = textureManager->Get(textureName);
        auto sharedTexture = texture.lock();
        if (!sharedTexture) {
            CE::Log(LogLevel::Error, "[Shader Manager] Texture '{}' was not found for shader uniform '{}'", textureName ? textureName : "", uniformName ? uniformName : "");
            return false;
        }

        renderer->SetShaderTexture(uniformName, sharedTexture.get(), slot);
        return true;
    }

    int ShaderManager::Debug_LoadedShadersCount() const {
        return static_cast<int>(shaders.size());
    }

    int ShaderManager::Debug_LoadedShadersNoError() const {
        int count = 0;
        for (const auto& [name, shaderInfo] : shaders) {
            (void)name;
            if (!shaderInfo.IsErrorShader) {
                ++count;
            }
        }
        return count;
    }

    int ShaderManager::Debug_LoadedShadersError() const {
        int count = 0;
        for (const auto& [name, shaderInfo] : shaders) {
            (void)name;
            if (shaderInfo.IsErrorShader) {
                ++count;
            }
        }
        return count;
    }

    std::string ShaderManager::Debug_GetBoundShaderName() const {
        return gBoundShaderName.empty() ? "Default" : gBoundShaderName;
    }

    std::vector<ShaderManager::DebugShaderInfo> ShaderManager::Debug_GetShaders() const {
        std::vector<DebugShaderInfo> debugShaders;
        debugShaders.reserve(this->shaders.size());

        for (const auto& [name, shaderInfo] : this->shaders) {
            DebugShaderInfo debugInfo{};
            debugInfo.name = name;
            debugInfo.vertexPath = shaderInfo.VertexPath;
            debugInfo.fragmentPath = shaderInfo.FragmentPath;
            debugInfo.usesDefaultVertex = shaderInfo.UsesDefaultVertex;
            debugInfo.usesDefaultFragment = shaderInfo.UsesDefaultFragment;
            debugInfo.isCompiled = shaderInfo.IsCompiled;
            debugInfo.isErrorShader = shaderInfo.IsErrorShader;
            debugInfo.isBound = (gBoundShaderName == name);
            debugShaders.push_back(debugInfo);
        }

        return debugShaders;
    }

    ShaderManager::~ShaderManager() {
        UnloadAll();
    }
}
