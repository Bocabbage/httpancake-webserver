#include "httpTcpConnection.hpp"

HttpTcpConnection::HttpTcpConnection(
    EventLoop* lp, string& name, int sockfd,
    const string &hostAddr, uint16_t hostPort,
    const string &peerAddr, uint16_t peerPort
):
TcpConnection(lp, name, sockfd, hostAddr, hostPort, peerAddr, peerPort),
processState_(PARSE_URL),
requestType_(NULL_METHOD),
mimeType_("default")
{

}

HttpTcpConnection::~HttpTcpConnection()
{  }