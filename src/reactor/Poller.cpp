#include "Poller.hpp"
#include "Handler.hpp"
#include <assert.h>

using std::vector;

Poller::Poller(EventLoop *lp):
epollFd_(::epoll_create1(EPOLL_CLOEXEC)),   // level-triggered
maxEventNum_(128),
events_(maxEventNum_),
ownerLp_(lp),
timeOut_(500)
{

}

Poller::~Poller()
{

}

void Poller::poll(vector<Handler*> &activeQueue)
{
    activeQueue.clear();

    int ret = ::epoll_wait(
        epollFd_, &(*events_.begin()), maxEventNum_, timeOut_
    );

    for(int i = 0; i < ret; ++i)
    {
        if(handlerList_.find(events_[i].data.fd) == handlerList_.end())
        {
            // crash
            // waiting for more graceful process
            exit(-1);
        }

        Handler* handler = handlerList_[events_[i].data.fd];
        handler->setRevents(events_[i].events);
        activeQueue.push_back(handler);
    }
}

void Poller::updateHandler(Handler *handler)
{
    int fd = handler->fd();
    if(handlerList_.find(fd) == handlerList_.end())
    {
        // add new-handler
        handlerList_[fd] = handler;
        ::epoll_event newEvents;
        newEvents.events = handler->events();
        newEvents.data.fd = fd;
        if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &newEvents) < 0)
        {
            // crash
            // waiting for more graceful process
            printf("%d", errno);
            exit(-1);
        }
    }
    else
    {
        // ensure the handler is the identical one
        // as the same-fd-handler in handlerList_
        assert(handler == handlerList_[fd]);

        ::epoll_event newEvents;
        newEvents.events = handler->events();
        newEvents.data.fd = fd;
        if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &newEvents) < 0)
        {
            // crash
            // waiting for more graceful process
            printf("%d", errno);
            exit(-1);
        }
    }
    

}

void Poller::removeHandler(Handler *handler)
{
    int fd = handler->fd();
    if(handlerList_.find(fd) == handlerList_.end())
    {
        // The handler to be removed is not in the
        // handlerList_.Just return silently.
        return;
    }

    if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) < 0)
    {
        // crash
        // waiting for more graceful process
        exit(-1);
    }

    handlerList_.erase(fd);
}