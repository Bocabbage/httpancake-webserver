#pragma once
#include <vector>
#include <memory>
#include <thread>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop*, int);
    ~EventLoopThreadPool();
    
    EventLoop* getNextLoop();
    void start();

private:
    EventLoop* baseLp_;
    std::vector<EventLoop*> lpQueue_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    int next_;
    bool started_;
    int threadNum_;
};