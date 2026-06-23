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
        mShaders.emplace(handle, info);
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

        if (!entry.Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] Failed to load shader: {}", filepath);
        } else {
            CE::Log(LogLevel::Debug, "[Shader Manager] Loaded shader: {}", filepath);
        }

        mShaders.emplace(handle, entry);
        return handle;
    }

    bool ShaderManager::LoadStage(ShaderHandle handle, const std::string& filepath, CE::Renderer::ShaderStage stage, int sampler_count) {
        ShaderEntry* entry = GetShaderEntry(handle);

        if (!entry) {
            CE::Log(LogLevel::Error, "[Shader Manager] LoadStage Invalid handle!");
            return false;
        }

        if (!entry->Shader) return false;

        if (filepath.empty()) {
            CE::Log(LogLevel::Error, "[Shader manager] LoadStage filepath was empty!");
            return false;
        }

        if (!mRenderer.LoadShaderStage(entry->Shader, filepath.c_str(), stage, sampler_count)) {
            CE::Log(LogLevel::Error, "[Shader Manager] Failed to load shader stage");
            entry->IsErrorShader = true;
            return false;
        }

        entry->IsErrorShader = false;
        entry->IsCompiled = false;

        if (stage == CE::Renderer::ShaderStage::Vertex) {
            entry->VertexPath = filepath;
            entry->UsesDefaultVertex = false;
        } else {
            entry->FragmentPath = filepath;
            entry->UsesDefaultFragment = false;
            entry->FragmentSamplerCount = sampler_count;
        }
        CE::Log(LogLevel::Debug, "[Shader Manager] Loaded shader stage: {}", filepath);
        return true;
    }

    bool ShaderManager::UseDefaultStage(ShaderHandle handle, CE::Renderer::ShaderStage stage) {
        ShaderEntry* entry = GetShaderEntry(handle);

        if (!entry, entry->Shader) {
            CE::Log(LogLevel::Error, "[Shader Manager] UseDefaultStage called for a missing shader: {}", handle.id);
            return false;
        }

        if (!mRenderer.UseDefaultShaderStage(entry->Shader, stage)) {
            
        }
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