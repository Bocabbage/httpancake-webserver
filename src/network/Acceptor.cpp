#include "Acceptor.hpp"
#include "socketUtils.hpp"
#include <netinet/in.h> // IPPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELAY
// #include "EventLoop.hpp"

Acceptor::Acceptor(EventLoop* lp, const string &hostAddr, uint16_t hostPort):
ownerLp_(lp),
listenSock_(sockets::createNonblockSocket()),
listening_(false),
handler_(listenSock_.fd(), lp)
{ 
    listenSock_.bind(hostAddr, hostPort);
    handler_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{  }

void Acceptor::listen()
{
    listenSock_.listen();
    listening_ = true;
    handler_.enableReading();    
}

void Acceptor::handleRead()
{
    string peerAddr;
    uint16_t peerPort;
    int connFd = listenSock_.accept(&peerAddr, &peerPort);

    // -------- disable Nagle's algorithm ------------- // 
    // int optval = 1;
    // ::setsockopt(connFd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

    if(connFd >= 0)
    {
        if(cb_)
            cb_(connFd, peerAddr, peerPort);
        else
            sockets::close(connFd);
    }

}