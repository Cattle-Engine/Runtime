#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Resources {
    using TextureHandle = uint64_t;
    
    // Inlined because I don't really want to pollute texture_manager.cpp
    class TextureRef {
        public:
            TextureRef() = default;

            TextureRef(TextureManager* mgr, TextureHandle handle, Texture* tex)
                : mManager(mgr), mHandle(handle), mTexture(tex) {}

            ~TextureRef() {
                if (mManager)
                    mManager->Return(mHandle);
            }

            Texture* Get() const { return mTexture; }

            bool IsValid() const {
                return mTexture != nullptr;
            }

            TextureRef(const TextureRef&) = delete;
            TextureRef& operator=(const TextureRef&) = delete;

            TextureRef(TextureRef&& other) noexcept {
                *this = std::move(other);
            }

            TextureRef& operator=(TextureRef&& other) noexcept {
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

    private:
        TextureManager* mManager = nullptr;
        TextureHandle mHandle = 0;
        Texture* mTexture = nullptr;
    };    
    
    class TextureManager {
        public:
            TextureManager(VFS::VFS& vfs, IRenderer& renderer);
            
            // Internal use for other systems only!
            TextureRef Acquire(TextureHandle handle);
            
            // Used to decrease RefCount in TextureEntry
            void Return(TextureHandle handle);
            
            // Returns InvalidTextureHandle if failure
            TextureHandle Load(std::string path);
            
            // When RefCount in TextureEntry a texture is fully deleted
            void Unload(TextureHandle handle);

            // Called at end of the frame
            void UnloadPendingDeletions();
        private:
            struct TextureEntry {
                Texture* Resource;
                uint32_t RefCount;
                std::string Path;
                bool IsError;
                bool IsPendingUnload;
            };

            // int because so I can check if it's valid
            TextureEntry* GetTextureEntry(TextureHandle handle);

            IRenderer& mRenderer;
            VFS::VFS& mVFS;
            uint64_t mNextHandleID = 0;
            std::unordered_map<TextureHandle, TextureEntry> mTextureCache;
    };
}