#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Assets::Shaders {
    class ShaderManager {
        public:
            struct DebugShaderInfo {
                std::string name;
                std::string vertexPath;
                std::string fragmentPath;
                bool usesDefaultVertex = true;
                bool usesDefaultFragment = true;
                bool isCompiled = false;
                bool isErrorShader = false;
                bool isBound = false;
            };

            ShaderManager(
                CE::Renderer::IRenderer* renderer,
                CE::VFS::VFS* vfs,
                CE::Renderer::Resources::TextureManager& textureManager
            );

            bool CreateProgram(const std::string& name);
            bool Load(const std::string& filepath, const std::string& name, int fragmentSamplerCount = 4);
            bool LoadStage(const std::string& name, const std::string& filepath, CE::Renderer::ShaderStage stage, int samplerCount = 1);
            bool UseDefaultStage(const std::string& name, CE::Renderer::ShaderStage stage);
            bool Compile(const std::string& name);
            bool Bind(const std::string& name);
            void Unbind();
            void Unload(const std::string& name);
            void UnloadAll();

            void SetFloat(const std::string& uniformName, float value);
            void SetVec2(const std::string& uniformName, float x, float y);
            void SetVec3(const std::string& uniformName, float x, float y, float z);
            void SetVec4(const std::string& uniformName, float x, float y, float z, float w);
            void SetMat4(const std::string& uniformName, const float* value);
            void SetInt(const std::string& uniformName, int value);
            bool SetTexture(const std::string& uniformName, const Renderer::Resources::TextureHandle texturehandle, int slot = 0);

            int Debug_LoadedShadersCount() const;
            int Debug_LoadedShadersNoError() const;
            int Debug_LoadedShadersError() const;
            std::string Debug_GetBoundShaderName() const;
            std::vector<DebugShaderInfo> Debug_GetShaders() const;

            ~ShaderManager();

        private:
            struct ManagedShader {
                bool IsErrorShader = false;
                bool ShownMissingError = false;
                bool IsCompiled = false;
                bool UsesDefaultVertex = true;
                bool UsesDefaultFragment = true;
                std::string ProgramPath;
                std::string VertexPath;
                std::string FragmentPath;
                CE::Renderer::Shader* Shader = nullptr;
                int FragmentSamplerCount = 4;
            };
            CE::Renderer::IRenderer* renderer = nullptr;
            CE::VFS::VFS* vfs = nullptr;
            CE::Renderer::Resources::TextureManager& mTextureManager;
            std::unordered_map<std::string, ManagedShader> shaders;
            std::string gBoundShaderName;

            const char* StageToString(CE::Renderer::ShaderStage stage) const;
            ManagedShader* FindShader(const std::string& name);
            const ManagedShader* FindShader(const std::string& name) const;
    };
}