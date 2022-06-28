#pragma once
#include <atomic>
#include <functional>
#include "Timestamp.hpp"


class Timer
{
    using TimerCallback = std::function<void()>;
public:
    Timer(const TimerCallback &callback, Timestamp when, double interval):
    callback_(callback), 
    expiration_(when), 
    interval_(interval), 
    repeat_(interval > 0.0),
    sequence_(++s_num_created_)
    {  }

    // Non-copyable
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void run() const { callback_(); }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

private:
    const double interval_;
    const TimerCallback callback_;
    const bool repeat_;
    Timestamp expiration_;

    static std::atomic<int64_t> s_num_created_;
    const int64_t sequence_;

};
