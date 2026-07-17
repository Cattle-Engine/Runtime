#include "engine/rendering/resources/shader_manager.hpp"

#include <algorithm>
#include <utility>

#include "engine/common/tracelog.hpp"

namespace {
    constexpr const char kDefaultVertexPath[] = "__ce_shader_manager_default_vertex";
    constexpr const char kDefaultFragmentPath[] = "__ce_shader_manager_default_fragment";
} // namespace

namespace CE::Renderer::Resources {
    ShaderRef::ShaderRef(ShaderManager *mgr, ShaderHandle handle, Shader *shader)
        : mManager(mgr), mHandle(handle), mShader(shader) {}

    ShaderRef::~ShaderRef() {
        if (mManager && mHandle) {
            mManager->Return(mHandle);
        }
    }

    ShaderRef::ShaderRef(ShaderRef &&other) noexcept {
        *this = std::move(other);
    }

    ShaderRef &ShaderRef::operator=(ShaderRef &&other) noexcept {
        if (this != &other) {
            if (mManager) {
                mManager->Return(mHandle);
            }

            mManager = other.mManager;
            mHandle = other.mHandle;
            mShader = other.mShader;
            other.mManager = nullptr;
            other.mHandle = ShaderHandle{};
            other.mShader = nullptr;
        }
        return *this;
    }

    void ShaderRef::Reset() {
        if (mManager && mHandle) {
            mManager->Return(mHandle);
        }

        mManager = nullptr;
        mHandle = ShaderHandle{};
        mShader = nullptr;
    }
} // namespace CE::Renderer::Resources

namespace CE::Renderer::Resources {
    ShaderManager::ShaderManager(VFS::VFS &vfs, IRenderer &renderer, TextureManager &tex_man)
        : mVFS(vfs), mRenderer(renderer), mTextureManager(tex_man) {}

    ShaderManager::ShaderEntry *ShaderManager::GetShaderEntry(ShaderHandle handle) {
        auto it = mShaders.find(handle);
        if (it != mShaders.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    ShaderManager::~ShaderManager() {
        for (auto &[name, entry] : mShaders) {
            if (entry.Shader) {
                mRenderer.UnloadShader(entry.Shader);
                entry.Shader = nullptr;
            }
        }

        mPendingUnloads.clear();
        mShaders.clear();
    }

    ShaderHandle ShaderManager::CreateProgram() {
        ShaderEntry info;
        ShaderHandle handle;

        info.Shader = mRenderer.CreateShaderProgram();
        info.IsErrorShader = info.Shader == nullptr;

        if (!info.Shader) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Failed to create shader program");
        }

        handle.id = mNextShaderHandleID++;
        mShaders.emplace(handle, std::move(info));
        return handle;
    }

    ShaderHandle ShaderManager::Load(const std::string &filepath, int fragment_sampler_count) {
        ShaderHandle handle;

        if (filepath.empty()) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Load file path was empty!");
            return handle;
        }

        ShaderEntry entry;

        handle.id = mNextShaderHandleID++;

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
            CE_LOG(LogLevel::Error, "[Shader Manager] Failed to load shader: {}", filepath);
        } else {
            CE_LOG(LogLevel::Debug, "[Shader Manager] Loaded shader: {}", filepath);
        }

        mShaders.emplace(handle, std::move(entry));
        return handle;
    }

    bool ShaderManager::LoadStage(ShaderHandle handle, const std::string &filepath, CE::Renderer::ShaderStage stage,
                                  int sampler_count) {
        ShaderEntry *entry = GetShaderEntry(handle);

        if (!entry) {
            CE_LOG(LogLevel::Error, "[Shader Manager] LoadStage Invalid handle!");
            return false;
        }

        if (!entry->Shader)
            return false;

        if (filepath.empty()) {
            CE_LOG(LogLevel::Error, "[Shader manager] LoadStage filepath was empty!");
            return false;
        }

        if (!mRenderer.LoadShaderStage(entry->Shader, filepath.c_str(), stage, sampler_count)) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Failed to load shader stage");
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
        CE_LOG(LogLevel::Debug, "[Shader Manager] Loaded shader stage: {}", filepath);
        return true;
    }

    bool ShaderManager::UseDefaultStage(ShaderHandle handle, CE::Renderer::ShaderStage stage) {
        ShaderEntry *entry = GetShaderEntry(handle);

        if (!entry || !entry->Shader) {
            CE_LOG(LogLevel::Error, "[Shader Manager] UseDefaultStage called for a missing shader: {}", handle.id);
            return false;
        }

        if (!mRenderer.UseDefaultShaderStage(entry->Shader, stage)) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Failed to set default stage for shader: {}", handle.id);
            return false;
        }

        entry->IsCompiled = false;

        if (stage == CE::Renderer::ShaderStage::Vertex) {
            entry->UsesDefaultVertex = true;
            entry->VertexPath = kDefaultVertexPath;
        } else {
            entry->UsesDefaultFragment = true;
            entry->FragmentPath = kDefaultFragmentPath;
        }

        return true;
    }

    bool ShaderManager::Compile(ShaderHandle handle) {
        ShaderEntry *entry = GetShaderEntry(handle);

        if (!entry || !entry->Shader) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Compile called for a missing shader: {}", handle.id);
            return false;
        }

        entry->IsCompiled = mRenderer.CompileShaderProgram(entry->Shader);
        entry->IsErrorShader = !entry->IsCompiled;

        return entry->IsCompiled;
    }

    bool ShaderManager::Bind(ShaderHandle handle) {
        ShaderEntry *entry = GetShaderEntry(handle);

        if (!entry || !entry->Shader) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Bind called for a missing shader: {}", handle.id);
            return false;
        }

        if (entry->IsPendingUnload) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Tried to bind a pending deletion shader");
            return false;
        }

        if (!entry->IsCompiled) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Bind called for an unbound shader: {}", handle.id);
            return false;
        }

        mRenderer.BindShader(entry->Shader);

        mBoundShaderID = handle;
        return true;
    }

    void ShaderManager::Unbind() {
        auto entry = GetShaderEntry(mBoundShaderID);
        if (entry) {
            entry->Texture.Reset();
        }
        mRenderer.UnbindShader();
        mBoundShaderID = ShaderHandle{};
    }

    void ShaderManager::Unload(ShaderHandle handle) {
        auto it = mShaders.find(handle);

        if (it == mShaders.end()) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Unload called for a stale or missing shader: {}", handle.id);
            return;
        }

        if (it->first.id == mBoundShaderID.id) {
            CE_LOG(LogLevel::Error,
                   "[Shader Manager] Unload called for a bound shader, PLEASE UNBIND IT BEFORE UNLOADING. ID: {}",
                   it->first.id);
            return;
        }

        if (it->second.IsPendingUnload) {
            CE_LOG(LogLevel::Warn, "[Shader manager] Shader has already been marked to be unloaded");
            return;
        }

        it->second.IsPendingUnload = true;
        mPendingUnloads.push_back(handle);
    }

    void ShaderManager::UnloadAll() {
        Unbind();
        for (auto &[name, entry] : mShaders) {
            entry.IsPendingUnload = true;
            mPendingUnloads.push_back(name);
        }
    }

    void ShaderManager::SetFloat(const std::string &uniformName, float value) {
        mRenderer.SetShaderFloat(uniformName.c_str(), value);
    }

    void ShaderManager::SetVec2(const std::string &uniformName, float x, float y) {
        mRenderer.SetShaderVec2(uniformName.c_str(), x, y);
    }

    void ShaderManager::SetVec3(const std::string &uniformName, float x, float y, float z) {
        mRenderer.SetShaderVec3(uniformName.c_str(), x, y, z);
    }

    void ShaderManager::SetVec4(const std::string &uniformName, float x, float y, float z, float w) {
        mRenderer.SetShaderVec4(uniformName.c_str(), x, y, z, w);
    }

    void ShaderManager::SetMat4(const std::string &uniformName, const float *value) {
        mRenderer.SetShaderMat4(uniformName.c_str(), value);
    }

    void ShaderManager::SetInt(const std::string &uniformName, int value) {
        mRenderer.SetShaderInt(uniformName.c_str(), value);
    }

    bool ShaderManager::SetTexture(const std::string &uniformName,
                                   const Renderer::Resources::TextureHandle texturehandle, int slot) {
        auto entry = GetShaderEntry(mBoundShaderID);
        Texture *tex = nullptr;

        if (!entry) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Tried to call SetTexture with no shader bound!");
            return false;
        }

        entry->Texture = mTextureManager.Acquire(texturehandle);
        tex = entry->Texture.Get();

        if (!tex) {
            CE_LOG(LogLevel::Error, "[Shader Manager] SetTexture texture handle was invalid");
            return false;
        }

        mRenderer.SetShaderTexture(uniformName.c_str(), tex, slot);
        return true;
    }

    size_t ShaderManager::Debug_LoadedShadersCount() const {
        return mShaders.size();
    }

    int ShaderManager::Debug_LoadedShadersNoError() const {
        int count = 0;
        for (const auto &[id, entry] : mShaders) {
            (void)id;
            if (!entry.IsErrorShader) {
                count++;
            }
        }
        return count;
    }

    int ShaderManager::Debug_LoadedShadersError() const {
        int count = 0;
        for (const auto &[id, entry] : mShaders) {
            (void)id;
            if (entry.IsErrorShader) {
                count++;
            }
        }
        return count;
    }

    std::vector<ShaderManager::DebugShaderInfo> ShaderManager::Debug_GetShaders() const {
        std::vector<DebugShaderInfo> debugShaders;
        debugShaders.reserve(this->mShaders.size());

        for (const auto &[id, shaderInfo] : this->mShaders) {
            DebugShaderInfo debugInfo{};
            debugInfo.id = id.id;
            debugInfo.vertexPath = shaderInfo.VertexPath;
            debugInfo.fragmentPath = shaderInfo.FragmentPath;
            debugInfo.usesDefaultVertex = shaderInfo.UsesDefaultVertex;
            debugInfo.usesDefaultFragment = shaderInfo.UsesDefaultFragment;
            debugInfo.isCompiled = shaderInfo.IsCompiled;
            debugInfo.isErrorShader = shaderInfo.IsErrorShader;
            debugInfo.isBound = (mBoundShaderID.id == id.id);
            debugShaders.push_back(debugInfo);
        }

        return debugShaders;
    }

    ShaderHandle ShaderManager::Debug_GetBoundShaderID() const {
        return mBoundShaderID;
    }

    void ShaderManager::Return(ShaderHandle handle) {
        auto entry = GetShaderEntry(handle);
        if (!entry) {
            CE_LOG(LogLevel::Error, "[Shader Manager] Return called with an invalid or stale handle");
            return;
        }

        if (entry->RefCount > 0)
            entry->RefCount--;
    }

    ShaderRef ShaderManager::AcquireRef(ShaderHandle handle) {
        auto entry = GetShaderEntry(handle);

        if (!entry) {
            CE_LOG(LogLevel::Error, "[Shader Manager] AcquireShaderRef Invalid or stale handle");
            return {};
        }

        if (entry->IsPendingUnload) {
            CE_LOG(LogLevel::Error, "[Shader Manager] AcquireShaderRef Shader is pending unload: {}", handle.id);
            return {};
        }

        if (!entry->IsCompiled) {
            CE_LOG(LogLevel::Error, "[Shader Manager] AcquireShaderRef Shader is not compiled, please use Compile");
            return {};
        }

        if (entry->IsErrorShader) {
            CE_LOG(LogLevel::Error, "[Shader Manager] AcquireShaderRef Shader is an error shader!");
        }

        entry->RefCount++;
        return ShaderRef(this, handle, entry->Shader);
    }

    void ShaderManager::UnloadPendingDeletions() {
        std::vector<ShaderHandle> still_pending;
        for (ShaderHandle handle : mPendingUnloads) {
            auto it = mShaders.find(handle);

            if (it == mShaders.end())
                continue;

            ShaderEntry &entry = it->second;

            if (entry.RefCount == 0) {
                if (entry.Shader) {
                    mRenderer.UnloadShader(entry.Shader);
                }
                mShaders.erase(it);
            } else {
                still_pending.push_back(handle);
            }
        }

        mPendingUnloads.swap(still_pending);
    }
} // namespace CE::Renderer::Resources