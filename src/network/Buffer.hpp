#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include <assert.h>


/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

// readable-bytes: 已经被写入 Buffer 的数据，可以被调用者读取
// writable-bytes: 从 read_fd() 读到的数据被写入此处（可自动增长）
// prependable-bytes：设计用于“挪动”：当writable-bytes空间不足、但实际这部分空间+writable-bytes空间可以容纳要写入的数据时，
//                    可以将readable-bytes数据直接前移，腾出足够的空间写入，无需再进行扩张；
//                    此外，该设计可以以很小的代价在数据前面添加少数字节（核心用处）：见Muduo7.4(p216)

class Buffer
{
// 作为值类型使用
// 不是线程安全的，但线程安全性被调用者所保证（assert_in_loop_thread）

public:
    static const size_t k_cheap_prepend = 8;   // prependable-bytes 的初始大小，同时也规定 prependable-bytes 最小值
    static const size_t k_init_size = 1024;    // writable-bytes 的初始大小

    Buffer():
    buffer_(k_init_size + k_cheap_prepend), 
    reader_idx_(k_cheap_prepend),
    write_idx_(k_cheap_prepend)
    {
        // 初始状态下，readable-bytes length = 0
    }

    // default copy-ctor, dtor and assignment are fine

    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(write_idx_, rhs.write_idx_);
        std::swap(reader_idx_, rhs.reader_idx_);
    }

    size_t readable_bytes() const { return write_idx_ - reader_idx_; }
    size_t writable_bytes() const { return buffer_.size() - write_idx_; }
    size_t prependable_bytes() const { return reader_idx_; }

    const char* peek() const { return begin() + reader_idx_; }

    void retrieve(size_t len)
    {
        // 前端被读走 len 字节
        assert(len <= readable_bytes());
        reader_idx_ += len;
    }

    void retrieve_until(const char *end)
    {
        assert(peek() <= end);
        assert(end <= begin_write());
        retrieve(end - peek());
    }

    void retrieve_all()
    {
        reader_idx_ = k_cheap_prepend;
        write_idx_ = k_cheap_prepend;
    }

    std::string retrieve_as_string()
    {
        std::string str(peek(), readable_bytes());
        retrieve_all();
        return str;
    }

    // Add by Zhuofan-Zhang
    std::string retrieve_line_as_string()
    {
        std::string result;
        std::string str(peek(), readable_bytes());
        size_t lfIdx = str.find('\n');
        if(lfIdx != std::string::npos)
        {
            result = str.substr(0, lfIdx);
            retrieve(lfIdx);
        }

        return result;
    }


    void append(const std::string &str)
    {
        append(str.data(), str.length());
    }

    void append(const char *data, size_t len)
    {
        ensure_writable_bytes(len);
        std::copy(data, data+len, begin_write());
        has_written(len);
    }

    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void ensure_writable_bytes(size_t len)
    {
        if(writable_bytes() < len)
            make_space(len);
        
        assert(writable_bytes() >= len);
    }

    char *begin_write() { return begin() + write_idx_; }
    const char *begin_write() const { return begin() + write_idx_; }

    void has_written(size_t len)
    {
        write_idx_ += len;
    }

    void prepend(const void *data, size_t len)
    {
        assert(len <= prependable_bytes());
        reader_idx_ -= len;
        const char *d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+reader_idx_);
    }

    void shrink(size_t reserve)
    {
        std::vector<char> buf(k_cheap_prepend+readable_bytes()+reserve);
        std::copy(peek(), peek()+readable_bytes(), buf.begin()+k_cheap_prepend);
        buf.swap(buffer_);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t read_fd(int fd, int *saved_errno);

private:

    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    void make_space(size_t len)
    {
        if(writable_bytes() + prependable_bytes() < len + k_cheap_prepend)
        {
            // 当前vector可写空间不足，必须动态增长
            // vector 自适应增长
            // capacity() 机制使buffer指数增长
            buffer_.resize(write_idx_ + len);
        }
        else
        {
            // readable-bytes 数据前移
            assert(k_cheap_prepend < reader_idx_);
            size_t readable = readable_bytes();
            std::copy(begin()+reader_idx_,
                      begin()+write_idx_,
                      begin()+k_cheap_prepend);
            reader_idx_ = k_cheap_prepend;
            write_idx_ = reader_idx_ + readable;
            assert(readable == readable_bytes());
        }
    }

    std::vector<char> buffer_;
    // 不使用迭代器而采用idx：当vector自动增长时迭代器会失效
    size_t reader_idx_;
    size_t write_idx_;
};


