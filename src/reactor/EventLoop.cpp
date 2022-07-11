#include "EventLoop.hpp"
#include "Poller.hpp"
#include "Handler.hpp"
#include "TimerQueue.hpp"
#include <sys/eventfd.h>
#include <signal.h>
#include <unistd.h>

class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }

};

IgnoreSigPipe initObj;

EventLoop::EventLoop():
stopLoop_(true),
callingPendingunctors_(false),
wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
wakeupHandler_(std::make_unique<Handler>(wakeupFd_, this)),
poller_(std::make_unique<Poller>(this)), // use factory-func?
timerQueue_(std::make_unique<TimerQueue>(this)),
tid_(std::this_thread::get_id())
{
    wakeupHandler_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
    wakeupHandler_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
}

void EventLoop::updateHandler(Handler *handler)
{
    poller_->updateHandler(handler);
}

void EventLoop::removeHandler(Handler *handler)
{
    poller_->removeHandler(handler);
}

void EventLoop::runInLoop(const InLoopFunction &f)
{
    if(isInLoopThread())
        f();    // call directly
    else
        queueInLoop(f);
}

void EventLoop::queueInLoop(const InLoopFunction &f)
{
    {
        // push into pending-task-queue
        std::lock_guard<std::mutex> guard(lpMutex_);
        pendingFuncs_.push_back(f);
    }

    if(!isInLoopThread() || callingPendingunctors_)
    {
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    int n = ::write(wakeupFd_, &one, sizeof(one));
    if(n < sizeof(one))
    {
        printf("EventLoop: wakeup failed.\n");
        exit(-1);
    }   
}

void EventLoop::handleWakeup()
{
    uint64_t one = 1;
    int n = ::read(wakeupFd_, &one, sizeof(one));
    if(n < sizeof(one))
    {
        printf("EventLoop: handleWakeup failed.\n");
        exit(-1);
    }
}

void EventLoop::doPendingFuncs_()
{
    std::vector<InLoopFunction> funcs;
    callingPendingunctors_ = true;

    {
        // swap-out the queue and release the mutex-lock
        std::lock_guard<std::mutex> guard(lpMutex_);
        funcs.swap(pendingFuncs_);
    }

    for(auto &func: funcs)
        func();
    
    callingPendingunctors_ = false;
}

void EventLoop::loop()
{
    stopLoop_ = false;
    while(!stopLoop_)
    {
        poller_->poll(activeQueue_);
        for(auto &h: activeQueue_)
            h->handleEvents();
        
        doPendingFuncs_();
    }

}

void EventLoop::stopLoop()
{
    stopLoop_ = true;
    if(!isInLoopThread())
        wakeup();
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerQueue_->add_timer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp tstamp(add_time(Timestamp::now(), delay));
    return timerQueue_->add_timer(cb, tstamp, 0.0);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp tstamp(add_time(Timestamp::now(), interval));
    return timerQueue_->add_timer(cb, tstamp, interval);
}

