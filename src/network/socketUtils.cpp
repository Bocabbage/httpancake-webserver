#include "socketUtils.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

void sockets::setNonblockandCloseonExe(int fd)
{
    int nowFlags = ::fcntl(fd, F_GETFD, 0);
    ::fcntl(fd, F_SETFD, nowFlags | SOCK_NONBLOCK | SOCK_CLOEXEC);
}

int sockets::createNonblockSocket()
{
    int fd = ::socket(
        AF_INET, 
        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
        0
    );
    if(fd < 0)
    {
        ::printf("sockets::createNonblockSocket error.\n");
        ::exit(-1);
    }
    // setNonblockandCloseonExe(fd);
    return fd;
}

void sockets::bind(int fd, string hostAddr, uint16_t port)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(hostAddr == "ANY")
        addr.sin_addr.s_addr = INADDR_ANY;
    else
    {
        int ret = ::inet_pton(AF_INET, hostAddr.c_str(), &(addr.sin_addr));
        if(ret < 0)
        {
            ::printf("sockets::bind inet_pton error.\n");
            ::exit(-1);
        }
    }

    int ret = ::bind(fd, (struct sockaddr *)(&addr), sizeof(addr));
    if(ret < 0)
    {
        ::printf("sockets::bind bind error.\n");
        ::exit(-1);
    }
}

void sockets::listen(int fd)
{
    int ret = ::listen(fd, sockets::backlog);
    if(ret < 0)
    {
        ::printf("sockets::listen error.\n");
        ::exit(-1);
    }
}

int sockets::accept(int listenFd, string *peerAddr, uint16_t* peerPort)
{
    struct sockaddr_in addr;
    socklen_t addrLen = static_cast<socklen_t>(sizeof (addr));
    int acceptFd = ::accept4(
        listenFd, (struct sockaddr *)(&addr), &addrLen,
        SOCK_NONBLOCK | SOCK_CLOEXEC);
    
    if(acceptFd < 0)
    {
        // ::printf("sockets::accpet error.\n");
        // ::exit(-1);
        return acceptFd;
    }

    char buff[INET_ADDRSTRLEN] = { 0 };
    ::inet_ntop(AF_INET, (void *)(&addr), buff, INET_ADDRSTRLEN);
    peerAddr->clear();
    peerAddr->append(buff);

    *peerPort = ntohs(addr.sin_port);

    return acceptFd;

}

void sockets::close(int fd)
{
    int ret = ::close(fd);
    if(ret < 0)
    {
        ::printf("sockets::close error.\n");
        ::exit(-1);
    }
}

void sockets::shutdownWrite(int fd)
{
    int ret = ::shutdown(fd, SHUT_WR);
    if(ret < 0)
    {
        ::printf("sockets::shutdownWrite error.\n");
        ::exit(-1);
    }
}
