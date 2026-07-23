#pragma once

#include <memory>
#include <string>

#include "engine/common/fs/vfs.hpp"

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

namespace CE::Assets {

    class VFSIOStream final : public Assimp::IOStream {
      public:
        VFSIOStream(VirtualFile* file, CE::VFS::VFS* vfs);
        ~VFSIOStream() override;

        size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override;
        size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override;

        aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;
        size_t Tell() const override;
        size_t FileSize() const override;
        void Flush() override;

      private:
        VirtualFile* mFile = nullptr;
        CE::VFS::VFS* mVFS = nullptr;
    };

    class VFSIOSystem final : public Assimp::IOSystem {
      public:
        explicit VFSIOSystem(CE::VFS::VFS* vfs);
        ~VFSIOSystem() override = default;

        bool Exists(const char* pFile) const override;
        char getOsSeparator() const override;

        Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override;
        void Close(Assimp::IOStream* pFile) override;

      private:
        CE::VFS::VFS* mVFS = nullptr;
    };

} // namespace CE::Assets