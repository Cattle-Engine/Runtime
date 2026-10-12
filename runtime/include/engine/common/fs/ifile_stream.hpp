#pragma once

#include "engine/common/fs/file_provider.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <streambuf>

namespace CE::Common::FS::VFS {
    class FileStreamBuf : public std::streambuf {
      public:
        explicit FileStreamBuf(IFile& file) : file_(file) {
            setg(input_buffer_.data(), input_buffer_.data(), input_buffer_.data());
            setp(output_buffer_.data(), output_buffer_.data() + output_buffer_.size());
        }

        ~FileStreamBuf() override {
            sync();
        }

      protected:
        int_type underflow() override {
            if (gptr() < egptr()) {
                return traits_type::to_int_type(*gptr());
            }

            if (!file_.IsOpen()) {
                return traits_type::eof();
            }

            const uint64_t position = file_.TellR();
            const uint64_t size = file_.Size();

            if (position >= size) {
                return traits_type::eof();
            }

            const uint64_t available = size - position;
            const size_t count = static_cast<size_t>(
                std::min<uint64_t>(available, input_buffer_.size())
            );

            if (count == 0 || !file_.Read(input_buffer_.data(), count)) {
                return traits_type::eof();
            }

            setg(
                input_buffer_.data(),
                input_buffer_.data(),
                input_buffer_.data() + count
            );

            return traits_type::to_int_type(*gptr());
        }

        int_type overflow(int_type ch = traits_type::eof()) override {
            if (!FlushOutput()) {
                return traits_type::eof();
            }

            if (!traits_type::eq_int_type(ch, traits_type::eof())) {
                *pptr() = traits_type::to_char_type(ch);
                pbump(1);
            }

            return traits_type::not_eof(ch);
        }

        std::streamsize xsputn(const char* data, std::streamsize count) override {
            if (count <= 0) {
                return 0;
            }

            std::streamsize written = 0;

            while (written < count) {
                const std::streamsize available = epptr() - pptr();

                if (available == 0) {
                    if (!FlushOutput()) {
                        break;
                    }
                    continue;
                }

                const std::streamsize chunk = std::min(
                    count - written,
                    available
                );

                std::copy_n(data + written, chunk, pptr());
                pbump(static_cast<int>(chunk));
                written += chunk;
            }

            return written;
        }

        int sync() override {
            return FlushOutput() ? 0 : -1;
        }

        pos_type seekoff(
            off_type offset,
            std::ios_base::seekdir direction,
            std::ios_base::openmode mode
        ) override {
            if (mode == std::ios_base::openmode{}) {
                return pos_type(off_type(-1));
            }

            if (!FlushOutput()) {
                return pos_type(off_type(-1));
            }

            // Calculate the target position before discarding the input buffer.
            int64_t base = 0;

            if (direction == std::ios_base::beg) {
                base = 0;
            } else if (direction == std::ios_base::end) {
                const uint64_t size = file_.Size();

                if (size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                    return pos_type(off_type(-1));
                }

                base = static_cast<int64_t>(size);
            } else if (direction == std::ios_base::cur) {
                if ((mode & std::ios_base::in) != 0) {
                    const uint64_t physical = file_.TellR();
                    const uint64_t unread = static_cast<uint64_t>(egptr() - gptr());

                    if (physical < unread ||
                        physical - unread > static_cast<uint64_t>(
                            std::numeric_limits<int64_t>::max()
                        )) {
                        return pos_type(off_type(-1));
                    }

                    base = static_cast<int64_t>(physical - unread);
                } else {
                    const uint64_t physical = file_.TellW();

                    if (physical > static_cast<uint64_t>(
                        std::numeric_limits<int64_t>::max()
                    )) {
                        return pos_type(off_type(-1));
                    }

                    base = static_cast<int64_t>(physical);
                }
            } else {
                return pos_type(off_type(-1));
            }

            if ((offset > 0 && base > std::numeric_limits<int64_t>::max() - offset) ||
                (offset < 0 && base < std::numeric_limits<int64_t>::min() - offset)) {
                return pos_type(off_type(-1));
            }

            const int64_t target = base + offset;

            if (target < 0) {
                return pos_type(off_type(-1));
            }

            if ((mode & std::ios_base::in) != 0 &&
                !file_.SeekR(target, SeekOrigin::Begin)) {
                return pos_type(off_type(-1));
            }

            if ((mode & std::ios_base::out) != 0 &&
                !file_.SeekW(target, SeekOrigin::Begin)) {
                return pos_type(off_type(-1));
            }

            setg(input_buffer_.data(), input_buffer_.data(), input_buffer_.data());
            setp(output_buffer_.data(), output_buffer_.data() + output_buffer_.size());

            return pos_type(target);
        }

        pos_type seekpos(
            pos_type position,
            std::ios_base::openmode mode
        ) override {
            return seekoff(
                static_cast<off_type>(position),
                std::ios_base::beg,
                mode
            );
        }

      private:
        bool FlushOutput() {
            const std::ptrdiff_t count = pptr() - pbase();

            if (count == 0) {
                return true;
            }

            if (!file_.IsOpen() ||
                !file_.Write(pbase(), static_cast<size_t>(count))) {
                return false;
            }

            setp(output_buffer_.data(), output_buffer_.data() + output_buffer_.size());
            return true;
        }

        IFile& file_;

        std::array<char, 4096> input_buffer_{};
        std::array<char, 4096> output_buffer_{};
    };

    class IStream : public std::istream {
      public:
        explicit IStream(std::unique_ptr<IFile> file)
            : std::istream(nullptr),
              file_(std::move(file)),
              buffer_(RequireFile(file_)) {
            rdbuf(&buffer_);
        }

        IFile* GetFile() const {
            return file_.get();
        }

      private:
        static IFile& RequireFile(const std::unique_ptr<IFile>& file) {
            if (!file) {
                throw std::invalid_argument("IStream requires a valid IFile");
            }

            return *file;
        }

        std::unique_ptr<IFile> file_;
        FileStreamBuf buffer_;
    };

    class OStream : public std::ostream {
      public:
        explicit OStream(std::unique_ptr<IFile> file)
            : std::ostream(nullptr),
              file_(std::move(file)),
              buffer_(RequireFile(file_)) {
            rdbuf(&buffer_);
        }

        IFile* GetFile() const {
            return file_.get();
        }

      private:
        static IFile& RequireFile(const std::unique_ptr<IFile>& file) {
            if (!file) {
                throw std::invalid_argument("OStream requires a valid IFile");
            }

            return *file;
        }

        std::unique_ptr<IFile> file_;
        FileStreamBuf buffer_;
    };

    class IOStream : public std::iostream {
      public:
        explicit IOStream(std::unique_ptr<IFile> file)
            : std::iostream(nullptr),
              file_(std::move(file)),
              buffer_(RequireFile(file_)) {
            rdbuf(&buffer_);
        }

        IFile* GetFile() const {
            return file_.get();
        }

      private:
        static IFile& RequireFile(const std::unique_ptr<IFile>& file) {
            if (!file) {
                throw std::invalid_argument("IOStream requires a valid IFile");
            }

            return *file;
        }

        std::unique_ptr<IFile> file_;
        FileStreamBuf buffer_;
    };
}