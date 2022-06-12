#pragma once
#include "FixBuffer.hpp"
#include <string>

using std::string;

class LogStream
{


public:
    using SmallFixBuffer = FixBuffer<smallBufferSize>;

    LogStream() = default;
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char c)
    {
        buffer_.append(string(1, c));
        return *this;
    }

    LogStream& operator<<(const char* str)
    {
        if(str)
        {
            buffer_.append(str);
        }
        else
        {
            buffer_.append("(null)");
        }

        return *this;
    }

    LogStream& operator<<(const string& str)
    {
        string msg(str);
        buffer_.append(std::move(msg));
        return *this;
    }

    const SmallFixBuffer& buffer() const { return buffer_; }
    SmallFixBuffer& buffer() { return buffer_; }
    void resetBuffer() { buffer_.reset(); }
    void append(const char* data) { buffer_.append(data); }

private:
    template<typename T>
    void formatInteger(T);

    void staticCheck();

    SmallFixBuffer buffer_;
    static const int maxNumberSize = 32;    
};
