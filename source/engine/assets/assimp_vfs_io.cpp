#include "engine/assets/assimp_vfs_io.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Assets {
    VFSIOStream::VFSIOStream(std::unique_ptr<CE::Common::FS::VFS::IFile> file) : mFile(std::move(file)) {}

    VFSIOStream::~VFSIOStream() = default;

    size_t VFSIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount) {
        if (!mFile || pSize == 0)
            return 0;
        return mFile->Read(pvBuffer, pSize * pCount) ? pCount : 0;
    }

    size_t VFSIOStream::Write(const void* pvBuffer, size_t pSize, size_t pCount) {
        if (!mFile || pSize == 0)
            return 0;
        return mFile->Write(pvBuffer, pSize * pCount) ? pCount : 0;
    }

    aiReturn VFSIOStream::Seek(size_t pOffset, aiOrigin pOrigin) {
        if (!mFile)
            return aiReturn_FAILURE;

        int64_t offset = static_cast<int64_t>(pOffset);

        CE::Common::FS::VFS::SeekOrigin origin = CE::Common::FS::VFS::SeekOrigin::Begin;
        if (pOrigin == aiOrigin_CUR)
            origin = CE::Common::FS::VFS::SeekOrigin::Current;
        else if (pOrigin == aiOrigin_END)
            origin = CE::Common::FS::VFS::SeekOrigin::End;

        bool success = mFile->SeekR(offset, origin);
        if (!success) {
            CE_LOG(LogLevel::Error, "[VFS] Seek failure at offset {}", offset);
        }

        return success ? aiReturn_SUCCESS : aiReturn_FAILURE;
    }

    size_t VFSIOStream::Tell() const {
        if (!mFile)
            return 0;
        return static_cast<size_t>(mFile->TellR());
    }

    size_t VFSIOStream::FileSize() const {
        if (!mFile)
            return 0;
        return static_cast<size_t>(mFile->Size());
    }

    void VFSIOStream::Flush() {
        if (mFile)
            mFile->Flush();
    }

    VFSIOSystem::VFSIOSystem(CE::Common::FS::VFS::VFS* vfs) : mVFS(vfs) {}

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

        auto file = mVFS->OpenFile(pFile);

        if (!file)
            return nullptr;

        return new VFSIOStream(std::move(file));
    }

    void VFSIOSystem::Close(Assimp::IOStream* pFile) {
        delete pFile;
    }

} // namespace CE::Assets
