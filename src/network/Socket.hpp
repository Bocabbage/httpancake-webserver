#pragma once
#include <string>

using std::string;

class Socket
{

public:
    explicit Socket(int sockfd): fd_(sockfd)
    {  }
    ~Socket();

    int fd() const { return fd_; }
    void listen();
    void bind(string hostAddr, uint16_t port);
    int accept(string* peerAddr, uint16_t* peerPort);

    void shutdownWrite();

private:
    int fd_;

};
