#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include "Timestamp.hpp"
#include "TimerId.hpp"

class Poller;
class Handler;
class TimerQueue;

class EventLoop
{

using InLoopFunction = std::function<void()>;
using TimerCallback = std::function<void()>;

public:
    EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    ~EventLoop();

    void loop();
    void stopLoop();

    void updateHandler(Handler *);
    void removeHandler(Handler *);
    void wakeup();
    void runInLoop(const InLoopFunction&);
    void queueInLoop(const InLoopFunction&);
    bool isInLoopThread() const { return tid_ == std::this_thread::get_id(); }
    std::thread::id tid() const { return tid_; }  // for test-and-debug

    // timers
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);

private:
    bool stopLoop_;
    bool callingPendingunctors_;

    int wakeupFd_;
    void handleWakeup();
    void doPendingFuncs_();
    std::unique_ptr<Handler> wakeupHandler_;

    std::thread::id tid_; // one-loop-per-thread
    std::mutex lpMutex_;

    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    std::vector<Handler*> activeQueue_;
    std::vector<InLoopFunction> pendingFuncs_;
};