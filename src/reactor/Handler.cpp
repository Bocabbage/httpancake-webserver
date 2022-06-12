#include "Handler.hpp"
#include "EventLoop.hpp"
#include <sys/epoll.h>

const int Handler::readEvent_ = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
const int Handler::writeEvent_ = EPOLLOUT;
const int Handler::closeEvent_ = EPOLLHUP;
const int Handler::errorEvent_ = EPOLLERR;

Handler::Handler(int fd, EventLoop* lp):
fd_(fd), ownerLp_(lp), events_(0), revents_(0)
{  }

Handler::~Handler()
{  }

void Handler::handleEvents()
{
    if((revents_ & errorEvent_) && errorCb_)
        errorCb_();
    
    if((revents_ & readEvent_) && readCb_)
        readCb_();
    
    if((revents_ & writeEvent_) && writeCb_)
        writeCb_();
    
    if((revents_ & closeEvent_) && closeCb_)
    {
        // for debug
        printf("close events: %d\n", revents_);

        closeCb_();
    }
}

void Handler::update()
{
    ownerLp_->updateHandler(this);
}