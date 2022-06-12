#pragma once
#include <vector>
#include <string>

const int smallBufferSize = 4000;
const int largeBufferSize = 4000 * 1000;

using std::vector;
using std::string;

template <int SIZE>
class FixBuffer
{
public:
    explicit FixBuffer():
        buffLen_(SIZE),
        buff_(),
        available_(0)
    {

    }

    ~FixBuffer() = default;

    bool empty()
    { return available_ == 0; }

    int available() const { return available_; }
    int free() const { return buffLen_ - available_; }

    // used only by LogStream
    void reset() { available_ = 0; }
    char* current() { return buff_ + available_; }
    void add(size_t len) { available_ += len; }

    string getContent()
    {
        string result(&(buff_[0]), &(buff_[0 + available_]));
        available_ = 0;
        return result;
    }

    void append(string &&msg)
    {
        for(int i = 0; i < msg.length(); ++i)
            buff_[i + available_] = msg[i];
        
        available_ += msg.length();
        buff_[available_] = '\0';
    }

private:
    size_t buffLen_;
    char buff_[SIZE];
    int available_;
};
