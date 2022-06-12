#pragma once
#include <sys/epoll.h>
#include <vector>
#include <unordered_map>

class Handler;
class EventLoop;

class Poller
{
public:
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;
    Poller(EventLoop*);
    ~Poller();

    void poll(std::vector<Handler*> &);
    void updateHandler(Handler *);
    void removeHandler(Handler *);

private:
    int epollFd_;
    int maxEventNum_;
    int timeOut_;
    std::vector<::epoll_event> events_;
    EventLoop* ownerLp_;
    // Attention: the Polloer obj don't "have" the handler obj,
    // So when a handler call its ~hanlder(), it should un-register here
    std::unordered_map<int, Handler*> handlerList_;

};