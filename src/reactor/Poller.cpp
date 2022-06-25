#include "Poller.hpp"
#include "Handler.hpp"
#include <unistd.h>
#include <assert.h>
#include <string.h>

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
    close(epollFd_);
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
    HandlerState hstate = handler->state();

    // register into the kernel
    if(hstate == HANDLER_NEW || hstate == HANDLER_DELETED)
    {
        
        if(hstate == HANDLER_NEW)
        {
            assert(handlerList_.find(fd) == handlerList_.end());
            handlerList_[fd] = handler;
        }
        else // HANDLER_DELETED
        {
            assert(handlerList_.find(fd) != handlerList_.end());
            assert(handlerList_[fd] == handler);
        }
        // add new-handler
        handlerList_[fd] = handler;
        ::epoll_event newEvents;
        bzero(&newEvents, sizeof(newEvents));
        newEvents.events = handler->events();
        newEvents.data.fd = fd;
        if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &newEvents) < 0)
        {
            // crash
            // waiting for more graceful process
            printf("%d", errno);
            exit(-1);
        }

        handler->setState(HANDLER_USING);
    }
    else 
    {
        // ensure the handler is the identical one
        // as the same-fd-handler in handlerList_
        assert(handler == handlerList_[fd]);
        assert(hstate == HANDLER_USING);

        if(handler->isNoneEvents())
        {
            if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) < 0)
            {
                printf("poll-updateHandler del error: errno-%d", errno);
                exit(-1);
            }
            handler->setState(HANDLER_DELETED);
        }
        else
        {
            ::epoll_event newEvents;
            bzero(&newEvents, sizeof(newEvents));
            newEvents.events = handler->events();
            newEvents.data.fd = fd;
            if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &newEvents) < 0)
            {
                // crash
                // waiting for more graceful process
                printf("poll-updateHandler mod error: errno-%d", errno);
                exit(-1);
            }
        }

    }
    

}

void Poller::removeHandler(Handler *handler)
{
    int fd = handler->fd();
    assert(handlerList_.find(fd) != handlerList_.end());
    assert(handlerList_[fd] == handler);
    assert(handler->isNoneEvents());

    HandlerState hstate = handler->state();
    assert(hstate == HANDLER_USING || HANDLER_DELETED);
    size_t n = handlerList_.erase(fd);
    assert(n==1);

    if(hstate == HANDLER_USING)
    {
        if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) < 0)
        {
            // crash
            // waiting for more graceful process
            exit(-1);
        }
    }
    
    handler->setState(HANDLER_NEW);

}