#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "Buffer.hpp"

class EventLoop;
class Socket;
class Handler;

using std::string;

class TcpConnection:
    public std::enable_shared_from_this<TcpConnection>
{
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

public:
    explicit TcpConnection(
        EventLoop* lp, string& name, int sockfd,
        const string &hostAddr, uint16_t hostPort,
        const string &peerAddr, uint16_t peerPort
    );
    virtual ~TcpConnection();

    bool connected() const { return state_ == CONNECTED; }
    string name() const { return connName_; }
    EventLoop* getLoop() { return lp_; }

    void send(const string& msg);
    void shutdown();
    void forceClose();
// should be called only once
// called by TcpServer
    void connectEstablished();
    void connectDestroyed();

// Callback setting
    void setConnectionCallback(ConnectionCallback ccb)
    { connCb_ = std::move(ccb); }
    void setMessageCallback(MessageCallback mcb)
    { messageCb_ = std::move(mcb); }
    void setWriteCompleteCallback(WriteCompleteCallback wcb)
    { writeCb_ = std::move(wcb); }
    void setCloseCallback( CloseCallback ccb)
    { closeCb_ = std::move(ccb); }

private:
    enum ConnState { CONNECTING, CONNECTED, DISCONNECTING, DISCONNECTED };

    void setState(ConnState s) { state_ = s; }
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const string&);
    void shutdownInLoop();

    EventLoop* lp_;
    string connName_;
    ConnState state_;
    std::unique_ptr<Socket> sock_;
    std::unique_ptr<Handler> handler_;

    string hostAddr_;
    uint16_t hostPort_;
    string peerAddr_;
    uint16_t peerPort_;

    ConnectionCallback connCb_;
    MessageCallback messageCb_;
    WriteCompleteCallback writeCb_;
    CloseCallback closeCb_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;