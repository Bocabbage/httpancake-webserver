#include "EventLoopThread.hpp"
#include "EventLoop.hpp"

EventLoopThread::EventLoopThread():
lp_(nullptr),
exiting_(false),
thread_(std::thread(std::bind(&EventLoopThread::threadFunc, this)))
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    lp_->stopLoop();
    thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
    while(lp_ == nullptr)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this]{ return lp_ != nullptr; });
    }

    return lp_;
}

void EventLoopThread::threadFunc()
{
    EventLoop lp;   // stack-var
    {
        std::lock_guard<std::mutex> lk(mutex_);
        lp_ = &lp;
        cond_.notify_one();
    }

    lp_->loop();
}
