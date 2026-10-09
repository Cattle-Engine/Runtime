#pragma once

#include <memory>
#include <string>
#include <utility>

#include "engine/common/fs/file_provider.hpp"

using SeekOrigin = CE::Common::FS::VFS::SeekOrigin;

namespace CE::Scripting::Bindings {
    class ASIFile {
        public:
            ASIFile(std::unique_ptr<CE::Common::FS::VFS::IFile> file)
                : mFile(std::move(file)) {}

        bool IsOpen() const {
            return mFile && mFile->IsOpen();
        }

        uint64_t Size() const {
            return mFile ? mFile->Size() : 0;
        }

        uint64_t TellR() {
            return mFile ? mFile->TellR() : 0;
        }

        bool SeekW(int64_t offset, SeekOrigin origin) {
            return mFile && mFile->SeekW(offset, origin);
        }

        bool SeekR(int64_t offset, SeekOrigin origin) {
            return mFile && mFile->SeekR(offset, origin);
        }

        bool Write(const void* buffer, size_t bytes) {
            return mFile && mFile->Write(buffer, bytes);
        }

        bool WriteString(const std::string& text) {
            return Write(text.data(), text.size());
        }

        bool Read(void* buffer, size_t bytes) {
            return mFile && mFile->Read(buffer, bytes);
        }

        std::string ReadString(uint64_t bytes) {
            if (!mFile || bytes == 0) return {};
            std::string result(static_cast<size_t>(bytes), '\0');
            return Read(result.data(), result.size()) ? result : std::string{};
        }

        int64_t GetDateModified() {
            return mFile ? mFile->GetDateModified() : 0;
        }

        uint64_t TellW() {
            return mFile ? mFile->TellW() : 0;
        }

        bool Flush() {
            return mFile && mFile->Flush();
        }

        bool Eof() const {
            return mFile ? mFile->Eof() : true;
        }

        private:
            std::unique_ptr<CE::Common::FS::VFS::IFile> mFile;
    };
}
