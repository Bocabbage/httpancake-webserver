#include "Socket.hpp"
#include "socketUtils.hpp"

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