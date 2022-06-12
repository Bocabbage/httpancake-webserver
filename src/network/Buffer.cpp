#include "Buffer.hpp"
#include <errno.h>
#include <memory.h>
#include <sys/uio.h>


ssize_t Buffer::read_fd(int fd, int *save_errno)
{
    char extrabuf[65536];
    const size_t writable = writable_bytes();

    // 使用 readv(int fd, const struct iovec *iov, int iovcnt);
    // 将 read 的内容投射到多个 buffer 中（scatter read）
    // 当当前buffer足够长时直接读入，否则将剩余部分读到栈上的临时buffer（extrabuf）
    // 再填充到可变长的buffer后；这样的设计有利于一次性尽可能多的读OS-buffer，减少系统调用的开销
    // 详见 UNP14.4 + Muduo7.4
    struct iovec vec[2];
    vec[0].iov_base = begin() + write_idx_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf); // if = 65536?

    const ssize_t n = readv(fd, vec, 2);
    if(n < 0)
    {
        *save_errno = errno;
    }
    else if(static_cast<size_t>(n) <= writable)
    {
        write_idx_ += n;
    }
    else
    {
        write_idx_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;

}