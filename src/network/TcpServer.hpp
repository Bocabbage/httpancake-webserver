#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "Buffer.hpp"
#include "EventLoopThreadPool.hpp"

using std::string;
using std::vector;
using std::map;

class EventLoop;
class Acceptor;
class TcpConnection;

class TcpServer
{
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnMap = map<string, TcpConnectionPtr>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

public:
    explicit TcpServer(EventLoop*, const string&, uint16_t, int);
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    ~TcpServer();

    void start();
    void setConnectionCallback(ConnectionCallback ccb)
    { connCb_ = std::move(ccb); }
    void setMessageCallback(MessageCallback mcb)
    { messageCb_ = std::move(mcb); }
    void setWriteCompleteCallback(WriteCompleteCallback wcb)
    { writeCb_ = std::move(wcb); }

    int connSize() const { return connections_.size(); }

private:
    void handleNewConnection(int sockfd, const string &peerAddr, uint16_t peerPort);
    void handleRemoveConnection(const TcpConnectionPtr&);
    void handleRemoveConnectionInLoop(const TcpConnectionPtr&);

    EventLoop *lp_;
    EventLoopThreadPool lpThreadPool_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnMap connections_;
    int nextConnId_;

    bool started_;
    string serverName_;

    ConnectionCallback connCb_;
    MessageCallback messageCb_;
    WriteCompleteCallback writeCb_;
};