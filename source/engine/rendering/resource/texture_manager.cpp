#include "engine/rendering/resources/texture_manager.hpp"
#include "engine/common/tracelog.hpp"
#include <format>

namespace {
    constexpr const char kGeneratedTexturePathName[] = "__ce_internal_texture_generated_path";
}

namespace CE::Renderer::Resources {
    TextureRef::TextureRef(TextureManager* mgr, TextureHandle handle, Texture* tex)
        : mManager(mgr), mHandle(handle), mTexture(tex) {}

    TextureRef::~TextureRef() {
        if (mManager && mHandle)
            mManager->Return(mHandle);
    }

    TextureRef::TextureRef(TextureRef&& other) noexcept {
        *this = std::move(other);
    }

    TextureRef& TextureRef::operator=(TextureRef&& other) noexcept {
        if (this != &other) {
            if (mManager) {
                mManager->Return(mHandle);
            }
            
            mManager = other.mManager;
            mHandle = other.mHandle;
            mTexture = other.mTexture;
            other.mManager = nullptr;
            other.mHandle = 0;
            other.mTexture = nullptr;
        }
        return *this;
    }
}

namespace CE::Renderer::Resources {
    TextureManager::TextureManager(VFS::VFS& vfs, IRenderer& renderer) : mVFS(vfs), mRenderer(renderer) {}

    TextureManager::TextureEntry* TextureManager::GetTextureEntry(TextureHandle handle) {
        auto list = mTextureCache.find(handle);
        if (list != mTextureCache.end()) {
            return &list->second;
        } 
        return nullptr;
    }

    TextureRef TextureManager::Acquire(TextureHandle handle) {
        TextureEntry* entry = GetTextureEntry(handle);
        if (!entry || entry->IsPendingUnload)
            return {};

        entry->RefCount++;
        return TextureRef(this, handle, entry->Resource);
    }

    void TextureManager::Return(TextureHandle handle) {
        TextureEntry* entry = GetTextureEntry(handle);
        if (entry) {
            if (entry->RefCount > 0) entry->RefCount--;
            return;
        }
    }

    TextureHandle TextureManager::Load(std::string path) {
        TextureEntry texentry = {};

        if (!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error, "[Texture Manager] File doesn't exist: {}", path);
            texentry.Resource = mRenderer.GetErrorTexture();
            texentry.IsError = true;
        } else {
            Texture* tex = mRenderer.LoadTex(path.c_str());

            if (tex == nullptr) {
                CE::Log(LogLevel::Error, "[Texture Manager] Failed to load texture: {}", path);
                texentry.Resource = mRenderer.GetErrorTexture();
                texentry.IsError = true;
            } else {
                texentry.Resource = tex;
                texentry.IsError = false;
            }
        }

        TextureHandle handle = mNextHandleID++;
        texentry.Path = path;
        texentry.RefCount = 0;
        mTextureCache.emplace(handle, std::move(texentry));
        return handle;
    }

    void TextureManager::Unload(TextureHandle handle) {
        auto it = mTextureCache.find(handle);
        if (it == mTextureCache.end()) {
            CE::Log(LogLevel::Error, "[Texture Manager] Tried to unload an already unloaded texture: {}", handle);
            return;
        }
        it->second.IsPendingUnload = true;
    }

    Texture* TextureManager::GetTexture(TextureHandle handle) {
        TextureEntry* entry = GetTextureEntry(handle);

        if (!entry) return nullptr;

        if (entry->IsPendingUnload) return nullptr;

        if (entry) {
            return entry->Resource;
        } else {
            return nullptr;
        }
    }

    void TextureManager::UnloadPendingDeletions() {
        for (auto it = mTextureCache.begin(); it != mTextureCache.end(); ) {
            TextureEntry& entry = it->second;

            if (entry.IsError) {
                it = mTextureCache.erase(it);
                continue;
            }

            if (entry.IsPendingUnload && entry.RefCount == 0) {
                if (entry.Resource) {
                    mRenderer.UnloadTex(entry.Resource);
                    entry.Resource = nullptr;
                }

                it = mTextureCache.erase(it);
            } else {
                ++it;
            }
        }
    }

    size_t TextureManager::GetLoadedTextureCount() const {
        return mTextureCache.size();
    }

    size_t TextureManager::GetValidTextureCount() const {
        size_t count = 0;

        for (const auto& [handle, entry] : mTextureCache) {
            if (!entry.IsError && !entry.IsPendingUnload) {
                ++count;
            }
        }

        return count;
    }

    size_t TextureManager::GetErrorTextureCount() const {
        size_t count = 0;

        for (const auto& [handle, entry] : mTextureCache) {
            if (entry.IsError) {
                ++count;
            }
        }

        return count;
    }

    size_t TextureManager::GetPendingUnloadCount() const {
        size_t count = 0;

        for (const auto& [handle, entry] : mTextureCache) {
            if (entry.IsPendingUnload) {
                ++count;
            }
        }

        return count;
    }

    TextureHandle TextureManager::CreateTextureFromData(
                    int width,
                    int height,
                    const void* pixels,
                    TextureFormat format,
                    int pitch = 0,
                    TextureFilter filter,
                    TextureWrap wrap
    ) {
        auto tex = mRenderer.CreateTextureFromData(
            width,
            height,
            pixels,
            format,
            pitch,
            filter,
            wrap
        );
        TextureEntry entry = {};
        
        if (tex == nullptr) {
            CE::Log(LogLevel::Error, "[Texture Manager] Failed to create a texture from data!");
            entry.IsError = true;
            entry.Resource = mRenderer.GetErrorTexture();
        } else {
            entry.IsError = false;
            entry.Resource = tex;
        }

        TextureHandle handle = mNextHandleID++;
        entry.Path = std::format("{}_{}", kGeneratedTexturePathName, handle);
        entry.IsPendingUnload = false;
        entry.RefCount = 0;
        mTextureCache.emplace(handle, std::move(entry));
        return handle;
    }
}