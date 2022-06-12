#pragma once
#include <functional>
#include "Handler.hpp"
#include "Socket.hpp"
#include <string>

class EventLoop;
using std::string;

class Acceptor
{

using NewConnCallback = std::function<
void(int sockfd, const string &peerAddr, uint16_t peerPort)
>;

public:
    explicit Acceptor(EventLoop*, const string&, uint16_t);
    ~Acceptor();

    void listen();
    bool listening() const 
    { return listening_; }

    void setNewConnCallback(NewConnCallback cb)
    { cb_ = std::move(cb); }

private:
    void handleRead();

    EventLoop* ownerLp_;
    Socket listenSock_;
    bool listening_;
    Handler handler_;
    NewConnCallback cb_;
};