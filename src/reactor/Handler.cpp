#include "Handler.hpp"
#include "EventLoop.hpp"
#include <sys/epoll.h>

const int Handler::readEvent_ = EPOLLIN | EPOLLPRI;
const int Handler::writeEvent_ = EPOLLOUT;

Handler::Handler(int fd, EventLoop* lp):
fd_(fd), ownerLp_(lp), state_(HANDLER_NEW), events_(0), revents_(0)
{  }

Handler::~Handler()
{  }

void Handler::handleEvents()
{
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN) && closeCb_)
        closeCb_();

    if((revents_ & EPOLLERR) && errorCb_)
        errorCb_();
    
    if((revents_ & (EPOLLIN | EPOLLPRI | EPOLLHUP)) && readCb_)
        readCb_();
    
    if((revents_ & EPOLLOUT) && writeCb_)
        writeCb_();
}

void Handler::update()
{
    ownerLp_->updateHandler(this);
}