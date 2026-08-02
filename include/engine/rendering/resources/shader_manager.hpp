#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Renderer::Resources {
    struct ShaderHandle {
        ShaderHandle() : id(0) {}
        ShaderHandle(const ShaderHandle& other) : id(other.id) {}

        uint64_t id = 0;


        ShaderHandle& operator=(const ShaderHandle& other) {
            id = other.id;
            return *this;
        }

        explicit operator bool() const {
            return id != 0;
        }

        bool operator==(const ShaderHandle& other) const {
            return id == other.id;
        }
    };

    struct ShaderHandleHash {
        size_t operator()(const ShaderHandle& handle) const {
            return std::hash<uint64_t>{}(handle.id);
        }
    };

    class ShaderManager;

    class ShaderRef {
      public:
        ShaderRef() = default;

        ShaderRef(ShaderManager* mgr, ShaderHandle handle, Shader* shader);

        ~ShaderRef();
        void Reset();
        Shader* Get() const {
            return mShader;
        }

        bool IsValid() const {
            return mShader != nullptr;
        }

        ShaderRef(const ShaderRef&) = delete;
        ShaderRef& operator=(const ShaderRef&) = delete;

        ShaderRef(ShaderRef&& other) noexcept;
        ShaderRef& operator=(ShaderRef&& other) noexcept;

      private:
        ShaderManager* mManager = nullptr;
        ShaderHandle mHandle{};
        Shader* mShader = nullptr;
    };

    class ShaderManager {
      public:
        struct DebugShaderInfo {
            uint64_t id;
            std::string vertexPath;
            std::string fragmentPath;
            bool usesDefaultVertex = true;
            bool usesDefaultFragment = true;
            bool isCompiled = false;
            bool isErrorShader = false;
            bool isBound = false;
        };

        ShaderManager(VFS::VFS& vfs, IRenderer& renderer, TextureManager& tex_man);

        /**
         * @brief For long term use of a shader, Eg using it in the material struct
         */
        ShaderRef AcquireRef(ShaderHandle handle);
        void Return(ShaderHandle handle);

        ShaderHandle CreateProgram();
        ShaderHandle Load(const std::string& filepath, int fragmentSamplerCount = 4);
        bool LoadStage(ShaderHandle handle, const std::string& filepath, CE::Renderer::ShaderStage stage,
                       int sampler_count = 1);
        bool UseDefaultStage(ShaderHandle handle, CE::Renderer::ShaderStage stage);
        bool Compile(ShaderHandle handle);
        bool Bind(ShaderHandle handle);
        void Unbind();
        /**
         * @brief Adds a shader to be deleted, if it's called on the current bound shader this will error
         */
        void Unload(ShaderHandle handle);
        /**
         * @brief Auto unbinds the shader and then marks all shaders for deletion
         */
        void UnloadAll();

        void SetFloat(const std::string& uniformName, float value);
        void SetVec2(const std::string& uniformName, float x, float y);
        void SetVec3(const std::string& uniformName, float x, float y, float z);
        void SetVec4(const std::string& uniformName, float x, float y, float z, float w);
        void SetMat4(const std::string& uniformName, const float* value);
        void SetInt(const std::string& uniformName, int value);
        bool SetTexture(const std::string& uniformName, const Renderer::Resources::TextureHandle texturehandle,
                        int slot = 0);

        size_t Debug_LoadedShadersCount() const;
        int Debug_LoadedShadersNoError() const;
        int Debug_LoadedShadersError() const;
        ShaderHandle Debug_GetBoundShaderID() const;
        std::vector<DebugShaderInfo> Debug_GetShaders() const;

        /**
         * @brief Called at the end of the frame to unload pending deletions
         */
        void UnloadPendingDeletions();

        ~ShaderManager();

      private:
        struct ShaderEntry {
            bool IsErrorShader = false;
            TextureRef Texture;
            bool ShownMissingError = false;
            bool IsCompiled = false;
            bool UsesDefaultVertex = true;
            bool UsesDefaultFragment = true;
            std::string ProgramPath;
            std::string VertexPath;
            std::string FragmentPath;
            CE::Renderer::Shader* Shader = nullptr;
            int FragmentSamplerCount = 4;
            uint64_t RefCount = 0;
            bool IsPendingUnload = false;
        };

        ShaderEntry* GetShaderEntry(ShaderHandle handle);

        VFS::VFS& mVFS;
        IRenderer& mRenderer;
        TextureManager& mTextureManager;
        uint64_t mNextShaderHandleID = 0;
        ShaderHandle mBoundShaderID;
        std::unordered_map<ShaderHandle, ShaderEntry, ShaderHandleHash> mShaders;
        std::vector<ShaderHandle> mPendingUnloads;
    };
} // namespace CE::Renderer::Resources