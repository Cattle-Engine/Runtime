#pragma once

#include <memory>
#include <utility>

#include "engine/common/fs/file_provider.hpp"

using SeekOrigin = CE::Common::FS::VFS::SeekOrigin;

namespace CE::Scripting::Bindings {
    class ASIFile {
        public:
            ASIFile(std::unique_ptr<CE::Common::FS::VFS::IFile> file) : mFile(std::move(file)) {}

            bool IsOpen();
            uint64_t Size();
            uint64_t TellR();
            bool SeekW(int64_t offset, SeekOrigin origin);
            bool SeekR(int64_t offset, SeekOrigin origin);
            int64_t GetDateModified();
            uint64_t TellW();
            bool Flush();
            bool Eof() const;
        private:
            std::unique_ptr<CE::Common::FS::VFS::IFile> mFile;
    };
}