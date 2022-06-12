#include "EventLoop.hpp"
#include "Poller.hpp"
#include "Handler.hpp"
#include <sys/eventfd.h>
#include <unistd.h>

EventLoop::EventLoop():
stopLoop_(true),
callingPendingunctors_(false),
wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
wakeupHandler_(new Handler(wakeupFd_, this)),
poller_(new Poller(this)), // use factory-func?
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
        // for debug
        // if(!isInLoopThread())
        // {
        //     printf("Call from other thread.\n");
        // }
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

