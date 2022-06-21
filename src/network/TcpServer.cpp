#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "TcpConnection.hpp"
#include "httpTcpConnection.hpp"
#include <arpa/inet.h>
#include <string.h>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// static tcpconnection factory function
template<typename... Args>
std::shared_ptr<TcpConnection> createConnection(connectionType connType, Args&&... args)
{
    if(connType == TCPCONN)
    {
        return std::make_shared<TcpConnection>(std::forward<Args>(args)...);
    }
    else if(connType == HTTPTCPCONN)
    {
        return std::make_shared<HttpTcpConnection>(std::forward<Args>(args)...);
    }

    // should not come here
    return nullptr;
}

TcpServer::TcpServer(
    EventLoop* lp, const string& hostAddr, uint16_t hostPort, int threadNum,
    connectionType connType
):
lp_(lp),
lpThreadPool_(lp_, threadNum),
nextConnId_(1),
acceptor_(std::make_unique<Acceptor>(lp_, hostAddr, hostPort)),
started_(false),
serverName_(hostAddr + ':' + std::to_string(hostPort)),
connType_(connType)
{
    acceptor_->setNewConnCallback(
        std::bind(&TcpServer::handleNewConnection, this, _1, _2, _3)
    );

    lpThreadPool_.start();
}

TcpServer::~TcpServer()
{

}

void TcpServer::start()
{
    if (!started_)
        started_ = true;

    if(!acceptor_->listening())
    {
        // NEED run-in-loop ?
        lp_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get())
        );
    }
}

void TcpServer::handleNewConnection(int sockfd, const string &peerAddr, uint16_t peerPort)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    ++nextConnId_;
    string connName = serverName_ + buf;

// get hostAddr info
    string hostAddr;
    uint16_t hostPort;

    struct sockaddr_in localAddr;
    bzero(&localAddr, sizeof(localAddr));
    socklen_t addrLen = sizeof(localAddr);
    if(::getsockname(sockfd, (struct sockaddr*)&localAddr, &addrLen) < 0)
    {
        printf("TcpServer::handlerNewConnection, getsockname failed.\n");
        exit(-1);
    }

    char buff[INET_ADDRSTRLEN] = { 0 };
    ::inet_ntop(AF_INET, (void *)(&localAddr), buff, INET_ADDRSTRLEN);
    hostAddr.append(buff);
    hostPort = ::ntohs(localAddr.sin_port);


// Create new TcpConnection obj, set the callback
    EventLoop* workLp = lpThreadPool_.getNextLoop();

    // TcpConnectionPtr conn = std::make_shared<TcpConnection>(
    //     workLp, connName, sockfd, hostAddr, hostPort, peerAddr, peerPort 
    // );
    TcpConnectionPtr conn = createConnection(
        connType_, 
        workLp, connName, sockfd, hostAddr, hostPort, peerAddr, peerPort
    );

    conn->setConnectionCallback(connCb_);
    conn->setMessageCallback(messageCb_);
    conn->setWriteCompleteCallback(writeCb_);
    conn->setCloseCallback(
        std::bind(&TcpServer::handleRemoveConnection, this, _1)
    );

    connections_[connName] = conn;
// enable read-func of conn's handler
    // conn->connectEstablished();
    workLp->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn)
    );

}

void TcpServer::handleRemoveConnection(const TcpConnectionPtr& connPtr)
{
    // size_t n = connections_.erase(connPtr->name());
    // if(n != 1)
    // {
    //     printf("TcpServer: handleRemoveConnection faild: n = %lu\n", n);
    //     exit(-1);
    // }

    // Must be put into pendingQueue
    // lp_->queueInLoop(
    //     std::bind(&TcpConnection::connectDestroyed, connPtr)
    // );
    lp_->runInLoop(
        std::bind(&TcpServer::handleRemoveConnectionInLoop, this, connPtr)
    );
}

void TcpServer::handleRemoveConnectionInLoop(const TcpConnectionPtr& connPtr)
{
    EventLoop *workLp = connPtr->getLoop();
    // remove from connections_ in mainLoop
    size_t n = connections_.erase(connPtr->name());
    if(n != 1)
    {
        printf("TcpServer: handleRemoveConnection faild: n = %lu\n", n);
        exit(-1);
    }
    workLp->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, connPtr)
    );
}