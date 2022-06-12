#pragma once
#include <string>
#include <sys/socket.h>

using std::string;

namespace sockets
{
    int createNonblockSocket();
    void listen(int fd);
    void setNonblockandCloseonExe(int fd);
    void bind(int fd, string hostAddr, uint16_t port);
    int accept(int listenFd, string *peerAddr, uint16_t* peerPort);
    void close(int fd);
    void shutdownWrite(int fd);

    const int backlog = SOMAXCONN;
}