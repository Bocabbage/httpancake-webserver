#pragma once
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include "TcpServer.hpp"
#include "httpTcpConnection.hpp"

using std::string;
using std::unordered_set;
using std::unordered_map;
using boost::circular_buffer;
class EventLoop;
class Buffer;

struct Entry
{
    explicit Entry(const std::weak_ptr<TcpConnection>& weakConn):
        weakConn_(weakConn)
    {  }

    ~Entry()
    {
        std::shared_ptr<TcpConnection> conn = weakConn_.lock();
        if(conn)
            conn->shutdown();
    }

    std::weak_ptr<TcpConnection> weakConn_;

};

using EntryPtr = std::shared_ptr<Entry>;
using Bucket = unordered_set<EntryPtr>;
using weakConnList = circular_buffer<Bucket>;

class webServer
{

public:
    explicit webServer(
        EventLoop* lp, 
        const string& hostAddr, uint16_t hostPort,
        string fileDir, 
        int threadNum=5,
        int expiredSec=-1
    );

    webServer(const webServer&) = delete;
    webServer& operator=(const webServer&) = delete;

    void start();

private:

    // parse function
    void parseRequestStartLine(string &&requestLine, HttpTcpConnection* httpConnPtr);
    void parseHeader(string &&headerLine, unordered_map<string, string>& headerContents);

    void responseToGet(HttpTcpConnection* httpConnPtr);
    void responseToPost(HttpTcpConnection* httpConnPtr);

    // call-back
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&, Buffer*);
    void onTimer();

    TcpServer server_;
    string fileDir_;    // directory that static-files located in

    int expiredSec_;
    weakConnList connBuckets_;

};