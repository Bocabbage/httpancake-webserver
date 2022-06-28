#pragma once

class Timer;

class TimerId
{
public:
    explicit TimerId(Timer* timer = NULL, int64_t seq = 0)
        : timer_(timer), sequence_(seq)
    {
    }

  // default copy-ctor, dtor and assignment are okay
    TimerId(const TimerId&) = default;
    ~TimerId() = default;

    friend class TimerQueue;

 private:
    Timer* timer_;
    int64_t sequence_;
};
