#include <algorithm>

#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Resources {
    ShaderManager::ShaderManager(VFS::VFS& vfs, IRenderer& renderer, TextureManager& tex_man) : mVFS(vfs), mRenderer(renderer), mTextureManager(tex_man) {}

    ShaderManager::ShaderEntry* ShaderManager::GetShaderEntry(ShaderHandle handle) {
        auto it = mShaders.find(handle);
        if (it != mShaders.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }
    
    void ShaderManager::Unbind() {
        mRenderer.UnbindShader();
        mBoundShaderID.id = 0;
    }

    ShaderHandle ShaderManager::CreateProgram() {
        ShaderEntry info;
        ShaderHandle handle;

        info.Shader = mRenderer.CreateShaderProgram();
        info.IsErrorShader = info.Shader == nullptr;

        if (!info.Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Failed to create shader program");
        }

        handle.id = mNextShaderHandleID++;
        return handle;
    }

    ShaderHandle ShaderManager::Load(const std::string& filepath, int fragment_sampler_count) {
        ShaderHandle handle;
        
        if (filepath.empty()) {
            CE::Log(LogLevel::Error, "[Shader Manager] Load file path was empty!");
            handle;
        }

        ShaderEntry entry;

        entry.ProgramPath = filepath;
        entry.VertexPath = filepath + ".vert";
        entry.FragmentPath = filepath + ".frag";
        entry.Shader = mRenderer.LoadShader(filepath.c_str(), fragment_sampler_count);
        entry.IsCompiled = entry.Shader != nullptr;
        entry.IsErrorShader = entry.Shader == nullptr;
        entry.UsesDefaultVertex = false;
        entry.UsesDefaultFragment = false;
        entry.FragmentSamplerCount = std::max(1, fragment_sampler_count);

    }

    void ShaderManager::Unload(ShaderHandle handle) {
        auto it = mShaders.find(handle);

        if (it == mShaders.end()) return;

        if (mBoundShaderID.id == it->first.id) {
            Unbind();
        }

        if (it->second.Shader) {
            mRenderer.UnloadShader(it->second.Shader);
            it->second.Shader = nullptr;
        }

        mShaders.erase(handle);
    }
}