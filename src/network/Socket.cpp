#include "Socket.hpp"
#include "socketUtils.hpp"
#include <netinet/in.h> // IPPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELAY

Socket::~Socket()
{
    sockets::close(fd_);
}

void Socket::listen()
{
    sockets::listen(fd_);
}

void Socket::bind(string hostAddr, uint16_t port)
{
    sockets::bind(fd_, hostAddr, port);
}

int Socket::accept(string *peerAddr, uint16_t *peerPort)
{
    return sockets::accept(fd_, peerAddr, peerPort);
}

void Socket::shutdownWrite()
{
    sockets::shutdownWrite(fd_);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}