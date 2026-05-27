#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/assets/textures.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

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
                CE::Assets::Textures::TextureManager* textureManager = nullptr
            );

            bool CreateProgram(const char* name);
            bool Load(const char* filepath, const char* name, int fragmentSamplerCount = 4);
            bool LoadStage(const char* name, const char* filepath, CE::Renderer::ShaderStage stage, int samplerCount = 1);
            bool UseDefaultStage(const char* name, CE::Renderer::ShaderStage stage);
            bool Compile(const char* name);
            bool Bind(const char* name);
            void Unbind();
            void Unload(const char* name);
            void UnloadAll();

            void SetFloat(const char* uniformName, float value);
            void SetVec2(const char* uniformName, float x, float y);
            void SetVec3(const char* uniformName, float x, float y, float z);
            void SetVec4(const char* uniformName, float x, float y, float z, float w);
            void SetMat4(const char* uniformName, const float* value);
            void SetInt(const char* uniformName, int value);
            bool SetTexture(const char* uniformName, const char* textureName, int slot = 0);

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
            CE::Assets::Textures::TextureManager* textureManager = nullptr;
            std::unordered_map<std::string, ManagedShader> shaders;
            std::string gBoundShaderName;

            const char* StageToString(CE::Renderer::ShaderStage stage) const;
            ManagedShader* FindShader(const char* name);
            const ManagedShader* FindShader(const char* name) const;
    };
}
