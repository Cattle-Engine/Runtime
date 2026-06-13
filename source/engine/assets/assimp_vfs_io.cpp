#include "engine/assets/assimp_vfs_io.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets {
    VFSIOStream::VFSIOStream(VirtualFile* file, CE::VFS::VFS* vfs)
        : mFile(file), mVFS(vfs) {
    }

    VFSIOStream::~VFSIOStream() {
        if (mFile && mVFS) {
            mVFS->CloseFile(mFile);
            mFile = nullptr;
        }
    }

    size_t VFSIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount) {
        if (!mFile || !mVFS)
            return 0;

        return mVFS->ReadFile(mFile, pvBuffer, pSize * pCount) / pSize;
    }

    size_t VFSIOStream::Write(const void* pvBuffer, size_t pSize, size_t pCount) {
        if (!mFile || !mVFS)
            return 0;

        return mVFS->WriteFile(mFile, pvBuffer, pSize * pCount) / pSize;
    }

    aiReturn VFSIOStream::Seek(size_t pOffset, aiOrigin pOrigin) {
        if (!mFile || !mVFS) return aiReturn_FAILURE;

        int64_t offset = static_cast<int64_t>(pOffset); 

        int whence = SEEK_SET;
        if (pOrigin == aiOrigin_CUR) whence = SEEK_CUR;
        else if (pOrigin == aiOrigin_END) whence = SEEK_END;

        bool success = mVFS->SeekFile(mFile, offset, whence);
        if (!success) {
            CE::Log(LogLevel::Error, "[VFS] Seek failure at offset {}", offset);
        }
        
        return success ? aiReturn_SUCCESS : aiReturn_FAILURE;
    }

    size_t VFSIOStream::Tell() const {
        if (!mFile || !mVFS)
            return 0;

        return (size_t)mVFS->TellFile(mFile);
    }

    size_t VFSIOStream::FileSize() const {
        if (!mFile || !mVFS)
            return 0;

        uint64_t size = 0;
        mVFS->GetFileSize(mFile->path.c_str(), size);
        return (size_t)size;
    }

    void VFSIOStream::Flush() {
        if (mFile && mVFS)
            mVFS->FlushFile(mFile);
    }

    VFSIOSystem::VFSIOSystem(CE::VFS::VFS* vfs)
        : mVFS(vfs) {
    }

    bool VFSIOSystem::Exists(const char* pFile) const {
        if (!mVFS)
            return false;

        return mVFS->FileExists(pFile);
    }

    char VFSIOSystem::getOsSeparator() const {
        return '/';
    }

    Assimp::IOStream* VFSIOSystem::Open(const char* pFile, const char* pMode) {
        (void)pMode;

        if (!mVFS)
            return nullptr;

        VirtualFile* file = mVFS->OpenFile(pFile, "rb");

        if (!file)
            return nullptr;

        return new VFSIOStream(file, mVFS);
    }

    void VFSIOSystem::Close(Assimp::IOStream* pFile) {
        delete pFile;
    }

}