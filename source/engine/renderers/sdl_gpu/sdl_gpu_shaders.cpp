#include <cctype>
#include <cstdlib>
#include <string>

#include <glm/gtc/type_ptr.hpp>

#include "engine/renderers/sdl_gpu_renderer.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        constexpr Uint32 kDefaultVertexUniformBufferCount = 1;
        constexpr Uint32 kCustomVertexUniformBufferCount = 2;
        constexpr Uint32 kDefaultFragmentSamplerCount = 1;
        constexpr Uint32 kCustomFragmentSamplerCount = 4;
        constexpr Uint32 kCustomFragmentUniformBufferCount = 1;

        bool TryParseSuffixIndex(const char* name, const char* prefix, size_t maxCount, size_t& indexOut) {
            if (!name || !prefix) {
                return false;
            }

            std::string nameString(name);
            std::string prefixString(prefix);
            if (!nameString.starts_with(prefixString)) {
                return false;
            }

            const std::string suffix = nameString.substr(prefixString.size());
            if (suffix.empty()) {
                return false;
            }

            char* endPtr = nullptr;
            const long parsedIndex = std::strtol(suffix.c_str(), &endPtr, 10);
            if (endPtr == nullptr || *endPtr != '\0' || parsedIndex < 0) {
                return false;
            }

            const size_t index = static_cast<size_t>(parsedIndex);
            if (index >= maxCount) {
                return false;
            }

            indexOut = index;
            return true;
        }

        std::string NormalizeUniformName(const char* name) {
            if (!name) {
                return {};
            }

            std::string normalized(name);
            for (char& ch : normalized) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            return normalized;
        }
    }

    SDL_GPU_Renderer_Shader* SDL_GPU_Renderer::GetShaderProgram(Shader* shaderProgram) {
        if (!shaderProgram) {
            return nullptr;
        }

        return static_cast<SDL_GPU_Renderer_Shader*>(shaderProgram->handle);
    }

    std::string SDL_GPU_Renderer::GetShaderBaseName(const char* path) {
        if (!path || path[0] == '\0') {
            return {};
        }

        std::string baseName(path);
        const size_t slash = baseName.find_last_of("/\\");
        if (slash != std::string::npos) {
            baseName.erase(0, slash + 1);
        }

        const std::string vertexSuffix = ".vert";
        const std::string fragmentSuffix = ".frag";

        if (baseName.size() > vertexSuffix.size() && baseName.ends_with(vertexSuffix)) {
            baseName.erase(baseName.size() - vertexSuffix.size());
        } else if (baseName.size() > fragmentSuffix.size() && baseName.ends_with(fragmentSuffix)) {
            baseName.erase(baseName.size() - fragmentSuffix.size());
        }

        return baseName;
    }

    bool SDL_GPU_Renderer::ParseIndexedUniformName(const char* name, const char* prefix, size_t maxCount, size_t& indexOut) {
        return TryParseSuffixIndex(name, prefix, maxCount, indexOut);
    }

    SDL_GPUShader* SDL_GPU_Renderer::GetStageShader(const SDL_GPU_Renderer_Shader* shaderProgram, ShaderStage stage) const {
        if (!shaderProgram) {
            return nullptr;
        }

        if (stage == ShaderStage::Vertex) {
            return shaderProgram->UsesDefaultVertex ? gDefaultVertexShader : shaderProgram->VertexShader;
        }

        return shaderProgram->UsesDefaultFragment ? gDefaultFragmentShader : shaderProgram->FragmentShader;
    }

    void SDL_GPU_Renderer::ReleaseProgramStage(SDL_GPU_Renderer_Shader* shaderProgram, ShaderStage stage) {
        if (!shaderProgram) {
            return;
        }

        SDL_GPUShader*& shaderHandle =
            (stage == ShaderStage::Vertex) ? shaderProgram->VertexShader : shaderProgram->FragmentShader;

        if (shaderHandle) {
            SDL_ReleaseGPUShader(gDevice, shaderHandle);
            shaderHandle = nullptr;
        }

        if (stage == ShaderStage::Vertex) {
            shaderProgram->VertexPath.clear();
        } else {
            shaderProgram->FragmentPath.clear();
        }
    }

    bool SDL_GPU_Renderer::LoadShaderStageIntoProgram(SDL_GPU_Renderer_Shader* shaderProgram, const char* path, ShaderStage stage) {
        if (!gDevice || !shaderProgram) {
            return false;
        }

        std::string shaderPath(path ? path : "");
        if (shaderPath.empty()) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] LoadShaderStage called with an empty path");
            return false;
        }

        ReleaseProgramStage(shaderProgram, stage);

        SDL_GPUShader* loadedShader = nullptr;
        if (stage == ShaderStage::Vertex) {
            loadedShader = Utils::LoadShader(
                gDevice,
                shaderPath,
                0,
                kCustomVertexUniformBufferCount,
                0,
                0,
                gVFS
            );
        } else {
            loadedShader = Utils::LoadShader(
                gDevice,
                shaderPath,
                kCustomFragmentSamplerCount,
                kCustomFragmentUniformBufferCount,
                0,
                0,
                gVFS
            );
        }

        if (!loadedShader) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] Failed to load {} shader '{}'",
                stage == ShaderStage::Vertex ? "vertex" : "fragment",
                shaderPath);
            return false;
        }

        if (stage == ShaderStage::Vertex) {
            shaderProgram->VertexShader = loadedShader;
            shaderProgram->UsesDefaultVertex = false;
            shaderProgram->VertexPath = shaderPath;
        } else {
            shaderProgram->FragmentShader = loadedShader;
            shaderProgram->UsesDefaultFragment = false;
            shaderProgram->FragmentPath = shaderPath;
        }

        shaderProgram->Dirty = true;
        return true;
    }

    Shader* SDL_GPU_Renderer::CreateShaderProgram() {
        auto* backendShader = new SDL_GPU_Renderer_Shader();
        auto* shaderProgram = new Shader();
        shaderProgram->handle = backendShader;
        shaderProgram->backend = gBackend;
        return shaderProgram;
    }

    Shader* SDL_GPU_Renderer::LoadShader(const char* path) {
        std::string baseName = GetShaderBaseName(path);
        if (baseName.empty()) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] LoadShader called with an invalid path");
            return nullptr;
        }

        Shader* shaderProgram = CreateShaderProgram();
        if (!shaderProgram) {
            return nullptr;
        }

        const bool loadedVertex = LoadShaderStage(shaderProgram, (baseName + ".vert").c_str(), ShaderStage::Vertex);
        const bool loadedFragment = LoadShaderStage(shaderProgram, (baseName + ".frag").c_str(), ShaderStage::Fragment);
        const bool compiled = loadedVertex && loadedFragment && CompileShaderProgram(shaderProgram);

        if (!compiled) {
            UnloadShader(shaderProgram);
            return nullptr;
        }

        CE::Log(LogLevel::Info, "[SDL_GPU Renderer] Loaded shader program '{}'", baseName);
        return shaderProgram;
    }

    bool SDL_GPU_Renderer::LoadShaderStage(Shader* shaderProgram, const char* path, ShaderStage stage) {
        auto* program = GetShaderProgram(shaderProgram);
        if (!program) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] LoadShaderStage called with an invalid shader program");
            return false;
        }

        return LoadShaderStageIntoProgram(program, path, stage);
    }

    bool SDL_GPU_Renderer::UseDefaultShaderStage(Shader* shaderProgram, ShaderStage stage) {
        auto* program = GetShaderProgram(shaderProgram);
        if (!program) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] UseDefaultShaderStage called with an invalid shader program");
            return false;
        }

        ReleaseProgramStage(program, stage);
        if (stage == ShaderStage::Vertex) {
            program->UsesDefaultVertex = true;
        } else {
            program->UsesDefaultFragment = true;
        }
        program->Dirty = true;
        return true;
    }

    bool SDL_GPU_Renderer::CompileShaderProgram(Shader* shaderProgram) {
        auto* program = GetShaderProgram(shaderProgram);
        if (!program) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] CompileShaderProgram called with an invalid shader program");
            return false;
        }

        SDL_Window* window = SDL_GetWindowFromID(mWindowID);
        if (!window) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] CompileShaderProgram could not resolve the renderer window");
            return false;
        }

        SDL_GPUShader* vertexShader = GetStageShader(program, ShaderStage::Vertex);
        SDL_GPUShader* fragmentShader = GetStageShader(program, ShaderStage::Fragment);
        if (!vertexShader || !fragmentShader) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] CompileShaderProgram requires both a vertex and fragment stage");
            return false;
        }

        SDL_GPUGraphicsPipeline* newPipeline = CreateGraphicsPipeline(window, vertexShader, fragmentShader);
        if (!newPipeline) {
            return false;
        }

        if (program->Pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, program->Pipeline);
        }
        program->Pipeline = newPipeline;
        program->Dirty = false;

        if (gCurrentShader == program) {
            BindActivePipeline();
            PushActiveShaderUniforms();
        }

        return true;
    }

    void SDL_GPU_Renderer::UnloadShader(Shader* shader) {
        if (!shader) {
            return;
        }

        auto* program = GetShaderProgram(shader);
        if (!program) {
            delete shader;
            return;
        }

        if (gCurrentShader == program) {
            gCurrentShader = nullptr;
            BindActivePipeline();
        }

        ReleaseProgramStage(program, ShaderStage::Vertex);
        ReleaseProgramStage(program, ShaderStage::Fragment);

        if (program->Pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gDevice, program->Pipeline);
            program->Pipeline = nullptr;
        }

        delete program;
        delete shader;
    }

    void SDL_GPU_Renderer::BindShader(Shader* shader) {
        if (!shader) {
            UnbindShader();
            return;
        }

        auto* program = GetShaderProgram(shader);
        if (!program) {
            CE::Log(LogLevel::Error, "[SDL_GPU Renderer] Invalid shader program passed to BindShader");
            return;
        }

        if (program->Dirty || !program->Pipeline) {
            if (!CompileShaderProgram(shader)) {
                CE::Log(LogLevel::Error, "[SDL_GPU Renderer] Failed to compile shader program before binding");
                return;
            }
        }

        gCurrentShader = program;
        BindActivePipeline();
        if (gCommandBuffer) {
            PushActiveShaderUniforms();
        }
    }

    void SDL_GPU_Renderer::UnbindShader() {
        gCurrentShader = nullptr;
        BindActivePipeline();
        if (gCommandBuffer) {
            PushActiveShaderUniforms();
        }
    }

    void SDL_GPU_Renderer::SetShaderFloat(const char* name, float value) {
        if (!gCurrentShader || !name) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        if (uniformName == "time") {
            gCurrentShader->Misc.x = value;
            return;
        }
        if (uniformName == "time2") {
            gCurrentShader->Misc.y = value;
            return;
        }
        if (uniformName == "time3") {
            gCurrentShader->Misc.z = value;
            return;
        }
        if (uniformName == "time4") {
            gCurrentShader->Misc.w = value;
            return;
        }

        size_t index = 0;
        if (ParseIndexedUniformName(uniformName.c_str(), "customfloat", gCurrentShader->CustomVec4.size() * 4, index)) {
            gCurrentShader->CustomVec4[index / 4][index % 4] = value;
        }
    }

    void SDL_GPU_Renderer::SetShaderVec2(const char* name, float x, float y) {
        if (!gCurrentShader || !name) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        if (uniformName == "resolution" || uniformName == "screensize") {
            gCurrentShader->Resolution = glm::vec4(x, y, gCurrentShader->Resolution.z, gCurrentShader->Resolution.w);
            return;
        }

        size_t index = 0;
        if (ParseIndexedUniformName(uniformName.c_str(), "customvec4", gCurrentShader->CustomVec4.size(), index)) {
            gCurrentShader->CustomVec4[index].x = x;
            gCurrentShader->CustomVec4[index].y = y;
        }
    }

    void SDL_GPU_Renderer::SetShaderVec3(const char* name, float x, float y, float z) {
        if (!gCurrentShader || !name) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        size_t index = 0;
        if (ParseIndexedUniformName(uniformName.c_str(), "customvec4", gCurrentShader->CustomVec4.size(), index)) {
            gCurrentShader->CustomVec4[index].x = x;
            gCurrentShader->CustomVec4[index].y = y;
            gCurrentShader->CustomVec4[index].z = z;
        }
    }

    void SDL_GPU_Renderer::SetShaderVec4(const char* name, float x, float y, float z, float w) {
        if (!gCurrentShader || !name) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        if (uniformName == "tint" || uniformName == "colour" || uniformName == "color") {
            gCurrentShader->Tint = glm::vec4(x, y, z, w);
            return;
        }

        size_t index = 0;
        if (ParseIndexedUniformName(uniformName.c_str(), "customvec4", gCurrentShader->CustomVec4.size(), index)) {
            gCurrentShader->CustomVec4[index] = glm::vec4(x, y, z, w);
        }
    }

    void SDL_GPU_Renderer::SetShaderMat4(const char* name, const float* mat4) {
        if (!gCurrentShader || !name || !mat4) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        const glm::mat4 matrix = glm::make_mat4(mat4);
        if (uniformName == "mvp") {
            gCurrentShader->OverrideMVP = matrix;
            gCurrentShader->HasOverrideMVP = true;
            return;
        }
        if (uniformName == "model") {
            gCurrentShader->ModelMatrix = matrix;
            return;
        }
        if (uniformName == "custommat4") {
            gCurrentShader->CustomMat4 = matrix;
        }
    }

    void SDL_GPU_Renderer::SetShaderInt(const char* name, int value) {
        if (!gCurrentShader || !name) {
            return;
        }

        const std::string uniformName = NormalizeUniformName(name);
        size_t index = 0;
        if (ParseIndexedUniformName(uniformName.c_str(), "customint", gCurrentShader->CustomInt4.size() * 4, index)) {
            gCurrentShader->CustomInt4[index / 4][index % 4] = value;
        }
    }

    void SDL_GPU_Renderer::SetShaderTexture(const char* name, Texture* texture, int slot) {
        (void)name;
        if (!gCurrentShader) {
            return;
        }

        if (slot < 0 || slot >= static_cast<int>(gCurrentShader->BoundTextures.size())) {
            CE::Log(LogLevel::Warn, "[SDL_GPU Renderer] SetShaderTexture slot {} is out of range", slot);
            return;
        }

        gCurrentShader->BoundTextures[static_cast<size_t>(slot)] = texture;
    }
}
