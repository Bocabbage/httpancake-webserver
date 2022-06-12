#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoopThread(EventLoopThread&) = delete;
    EventLoopThread& operator=(EventLoopThread&) = delete;

    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *lp_;
    bool exiting_;
    std::thread thread_;
    mutable std::mutex mutex_;  // protect the lp_
    std::condition_variable cond_;
    
};