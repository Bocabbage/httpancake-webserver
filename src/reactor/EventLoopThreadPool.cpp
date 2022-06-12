#include "EventLoopThreadPool.hpp"
#include "EventLoop.hpp"
#include "EventLoopThread.hpp"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLp, int threadNum=2):
baseLp_(baseLp),
threadNum_(threadNum),
started_(false),
next_(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    if(lpQueue_.empty())
    {
        printf("EventLoopThreadPool: getNextLoop failed.\n");
        exit(-1);
    }

    EventLoop* rLp = lpQueue_[next_];
    next_++;
    if(next_ == lpQueue_.size())
        next_ = 0;
    
    return rLp;
}

void EventLoopThreadPool::start()
{
    if(started_)
    {
        printf("EventLoopThreadPool::start : Start twice.\n");
        exit(-1);
    }

    started_ = true;
    for(int i = 0; i < threadNum_; ++i)
    {
        std::unique_ptr<EventLoopThread> nt(new EventLoopThread);
        threads_.push_back(std::move(nt));
        lpQueue_.push_back(threads_[i]->startLoop());
    }
}