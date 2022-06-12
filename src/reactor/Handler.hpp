#pragma once
#include <functional>

class EventLoop;

class Handler
{
    using EventCallback = std::function<void()>;

public:
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler(int fd, EventLoop*);
    ~Handler();

    /* Set callback function */
    void setReadCallback(EventCallback rcb)
    { readCb_ = std::move(rcb); }

    void setWriteCallback(EventCallback wcb)
    { writeCb_ = std::move(wcb); }

    void setErrorCallback(EventCallback ecb)
    { errorCb_ = std::move(ecb); }

    void setCloseCallback(EventCallback ccb)
    { closeCb_ = std::move(ccb); }

    /* Basic public accessment */
    void setRevents(int revents)
    { revents_ = revents; }
    int fd() const
    { return fd_; }
    int events() const
    { return events_; }


    /* Enable/Disable events */
    void enableReading()
    { events_ |= readEvent_; update(); }
    void enableWriting()
    { events_ |= writeEvent_; update(); }
    void disableReading()
    { events_ &= ~readEvent_; update(); }
    void disableWriting()
    { events_ &= ~writeEvent_; update(); }
    void disableAll()
    { events_ = 0; update(); }

    /* event-state check */
    bool isWriting() const { return events_ & writeEvent_; }

    void handleEvents();

private:
    void update();

    static const int readEvent_;
    static const int writeEvent_;
    static const int closeEvent_;
    static const int errorEvent_;

    int fd_;
    int events_;
    int revents_;
    EventCallback readCb_;
    EventCallback writeCb_;
    EventCallback closeCb_;
    EventCallback errorCb_;

    EventLoop* ownerLp_;
};