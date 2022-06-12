#include "TcpConnection.hpp"
#include "EventLoop.hpp"
#include "Handler.hpp"
#include "Socket.hpp"
#include <unistd.h>
// #include "socketUtils.hpp"

TcpConnection::TcpConnection(
    EventLoop* lp, string& name, int sockfd,
    const string &hostAddr, uint16_t hostPort,
    const string &peerAddr, uint16_t peerPort
):
lp_(lp), connName_(name),
sock_(new Socket(sockfd)),
handler_(new Handler(sockfd, lp)),
state_(CONNECTING),
hostAddr_(hostAddr), hostPort_(hostPort),
peerAddr_(peerAddr), peerPort_(peerPort)
{
    handler_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this)
    );

    handler_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );

    handler_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );

    handler_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );
}

TcpConnection::~TcpConnection()
{

}

void TcpConnection::send(const string& msg)
{
    if(state_ == CONNECTED)
    {
        if(lp_->isInLoopThread())
            sendInLoop(msg);
        else
        {
            // for debug
            // printf("***********************\n");
            // printf("TcpConnection::send: Not inLoop\n");
            // printf("WorkLp tid: %d\n", lp_->tid());
            // printf("Current tid: %d\n", std::this_thread::get_id());
            // printf("***********************\n");

            lp_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, msg)
            );
        }
            
    }
}

void TcpConnection::shutdown()
{
    if(state_ == CONNECTED)
    {
        setState(DISCONNECTING);
        lp_->runInLoop(
                std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}

void TcpConnection::connectEstablished()
{
    if(state_ != CONNECTING)
    {
        printf("connectionEstablished faild: state != CONNECTING\n");
        exit(-1);
    }
    setState(CONNECTED);
    handler_->enableReading();
    connCb_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_ != CONNECTING && state_ != CONNECTED)
    {
        printf("connectionDestroyed faild: state != CONNECTING/CONNECTED\n");
        printf("state = %d\n", state_);
        exit(-1);
    }
    setState(DISCONNECTED);
    handler_->disableAll();
    connCb_(shared_from_this());

    lp_->removeHandler(handler_.get());
}

void TcpConnection::handleRead()
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.read_fd(handler_->fd(), &saveErrno);
    if(n > 0)
    {
        messageCb_(shared_from_this(), &inputBuffer_);
    }
    else if(n == 0)
    {
        // for debug
        printf("handleRead: n == 0\n");
        handleClose();
    }
    else
    {
        errno = saveErrno;
        handleError();
    }
}

void TcpConnection::handleClose()
{
    if(state_ != CONNECTED)
    {
        printf("TcpConnection::handleClose failed.\n");
        exit(-1);
    }
    handler_->disableAll();

    closeCb_(shared_from_this());
}

void TcpConnection::handleWrite()
{
    if(handler_->isWriting())
    {
        ssize_t n = ::write(
            handler_->fd(),
            outputBuffer_.peek(),
            outputBuffer_.readable_bytes()
        );

        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readable_bytes() == 0)
            {
                // finish writing, disable the write-event
                handler_->disableWriting();
                if(writeCb_)
                {
                    lp_->queueInLoop(std::bind(writeCb_, shared_from_this()));
                }
                if(state_ == DISCONNECTING)
                    shutdownInLoop();
            }
        }
    }
}

void TcpConnection::handleError()
{

}

void TcpConnection::sendInLoop(const string& msg)
{
    ssize_t n = 0;
    if(!handler_->isWriting() && outputBuffer_.readable_bytes() == 0)
    {
        n = ::write(handler_->fd(), msg.data(), msg.size());
        if(n >= 0)
        {
            // write the complete data in once time
            if(n == msg.size() && writeCb_)
                lp_->queueInLoop(std::bind(writeCb_, shared_from_this()));
        }
        else
        {
            n = 0;
        }
    }

    if(n < msg.size())
    {
        outputBuffer_.append(msg.data() + n, msg.size() - n);
        if(!handler_->isWriting())
            handler_->enableWriting();
    }

}

void TcpConnection::shutdownInLoop()
{
    if(!handler_->isWriting())
    {
        sock_->shutdownWrite();
    }
}