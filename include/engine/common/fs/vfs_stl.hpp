#pragma once

#include "engine/common/fs/vfs.hpp"

#include <istream>
#include <memory>
#include <streambuf>
#include <vector>

namespace CE::VFS {

class VfsStreamBuf final : public std::streambuf {
public:
    VfsStreamBuf(VFS& vfs, VirtualFile* file, size_t buffer_size = 4096)
        : vfs_(&vfs), file_(file), buffer_(buffer_size)
    {
        if (buffer_.empty())
            buffer_.resize(1);
        reset_get_area();
    }

    void set_file(VirtualFile* file)
    {
        file_ = file;
        reset_get_area();
    }

protected:
    int_type underflow() override
    {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());
        if (!vfs_ || !file_)
            return traits_type::eof();

        const size_t n = vfs_->ReadFile(file_, buffer_.data(), buffer_.size());
        if (n == 0)
            return traits_type::eof();

        setg(buffer_.data(), buffer_.data(), buffer_.data() + static_cast<std::ptrdiff_t>(n));
        return traits_type::to_int_type(*gptr());
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
    {
        if (!(which & std::ios_base::in))
            return pos_type(off_type(-1));
        if (!vfs_ || !file_)
            return pos_type(off_type(-1));

        // Adjust "current" position to account for unread bytes in the get area.
        const std::ptrdiff_t buffered = egptr() - gptr();
        const int64_t base_pos = vfs_->TellFile(file_);
        if (base_pos < 0)
            return pos_type(off_type(-1));

        int64_t target_pos = 0;
        switch (dir) {
            case std::ios_base::beg:
                target_pos = static_cast<int64_t>(off);
                break;
            case std::ios_base::cur:
                target_pos = (base_pos - static_cast<int64_t>(buffered)) + static_cast<int64_t>(off);
                break;
            case std::ios_base::end:
                // Use the VFS SEEK_END implementation.
                if (!vfs_->SeekFile(file_, static_cast<int64_t>(off), SEEK_END))
                    return pos_type(off_type(-1));
                reset_get_area();
                return pos_type(static_cast<off_type>(vfs_->TellFile(file_)));
            default:
                return pos_type(off_type(-1));
        }

        if (!vfs_->SeekFile(file_, target_pos, SEEK_SET))
            return pos_type(off_type(-1));

        reset_get_area();
        return pos_type(static_cast<off_type>(vfs_->TellFile(file_)));
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
        if (!(which & std::ios_base::in))
            return pos_type(off_type(-1));
        if (!vfs_ || !file_)
            return pos_type(off_type(-1));

        const auto target = static_cast<int64_t>(pos);
        if (!vfs_->SeekFile(file_, target, SEEK_SET))
            return pos_type(off_type(-1));

        reset_get_area();
        return pos;
    }

private:
    void reset_get_area() { setg(buffer_.data(), buffer_.data(), buffer_.data()); }

    VFS* vfs_;
    VirtualFile* file_;
    std::vector<char> buffer_;
};

class VfsIStream final : public std::istream {
public:
    explicit VfsIStream(VFS& vfs, const char* virtual_path, size_t buffer_size = 4096)
        : std::istream(nullptr),
          vfs_(&vfs),
          file_(vfs.OpenFile(virtual_path)),
          buf_(vfs, file_, buffer_size)
    {
        if (!file_) {
            setstate(std::ios_base::failbit);
            return;
        }
        rdbuf(&buf_);
    }

    ~VfsIStream() override
    {
        if (vfs_ && file_)
            vfs_->CloseFile(file_);
        file_ = nullptr;
    }

    VfsIStream(const VfsIStream&) = delete;
    VfsIStream& operator=(const VfsIStream&) = delete;
    VfsIStream(VfsIStream&&) = delete;
    VfsIStream& operator=(VfsIStream&&) = delete;

    bool is_open() const { return file_ != nullptr; }

private:
    VFS* vfs_;
    VirtualFile* file_;
    VfsStreamBuf buf_;
};

inline std::unique_ptr<VfsIStream> OpenIStream(VFS& vfs, const char* virtual_path, size_t buffer_size = 4096)
{
    auto stream = std::make_unique<VfsIStream>(vfs, virtual_path, buffer_size);
    if (!stream->is_open())
        return nullptr;
    return stream;
}

} // namespace CE::VFS

