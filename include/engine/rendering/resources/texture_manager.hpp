#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Resources {
    using TextureHandle = uint64_t;
    
    class TextureManager; // Forward declaration
    
    class TextureRef {
    public:
        TextureRef() = default;
        
        TextureRef(TextureManager* mgr, TextureHandle handle, Texture* tex);
        
        ~TextureRef(); // Not inline anymore
        
        Texture* Get() const { return mTexture; }
        
        bool IsValid() const {
            return mTexture != nullptr;
        }
        
        TextureRef(const TextureRef&) = delete;
        TextureRef& operator=(const TextureRef&) = delete;
        
        TextureRef(TextureRef&& other) noexcept;
        TextureRef& operator=(TextureRef&& other) noexcept;
        
    private:
        TextureManager* mManager = nullptr;
        TextureHandle mHandle = 0;
        Texture* mTexture = nullptr;
    };  
    
    class TextureManager {
    public:
        TextureManager(VFS::VFS& vfs, IRenderer& renderer);
        ~TextureManager();
        // Internal use for other systems only!
        TextureRef Acquire(TextureHandle handle);
        Texture* GetTexture(TextureHandle handle);
        
        // Used to decrease RefCount in TextureEntry
        void Return(TextureHandle handle);
        
        /*
        * @brief Loads a texture from the VFS and returns a TextureHandle
        * @param path The path to the texture
        */
        TextureHandle Load(std::string path);
        
        /*
        * @brief Upload raw pixels to the GPU and get a texture
        * @param width The width of the texture
        * @param height The height of the texture
        * @param pixels Raw pixel data
        * @param pitch How many bytes between one row and another
        * @param filter There is: Nearest and Linear, Nearest keeps pixels sharp, Linear blurs them a tiny bit 
        * @param wrap There is: Clamp, Repeat and Mirror, Clamp stretchs edge pixels, Repeat tiles the texture, MirroredRepeat tiles but mirrors each time
        * @param cache_key Used to optimize, if a texture is already loaded with the same cache key, it returns that instead of creating an entirely new texture
        */
        TextureHandle CreateTextureFromData(
                int width,
                int height,
                const void* pixels,
                TextureFormat format,
                int pitch = 0,
                TextureFilter filter = TextureFilter::Linear,
                TextureWrap wrap = TextureWrap::Clamp,
                std::string cache_key = ""
        );
        
        /*
        * @brief This marks a texture for deletion and doesn't allow you to get the texture anymore, a texture is only deleted when its RefCount is 0
        */
        void Unload(TextureHandle handle);
        void UnloadAll();
        
        /*
        * @brief Find if a path has been cached
        */
        bool IsPathCached(std::string path);

        size_t GetLoadedTextureCount() const;
        size_t GetValidTextureCount() const;
        size_t GetErrorTextureCount() const;
        size_t GetPendingUnloadCount() const;
        
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
        std::unordered_map<std::string, TextureHandle> mPathCache;
    };
}