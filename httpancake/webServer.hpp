#pragma once
#include <string>
#include "TcpServer.hpp"
#include "TcpConnection.hpp"

using std::string;
class EventLoop;
class Buffer;

class webServer
{

public:
    explicit webServer(
        EventLoop* lp, 
        const string& hostAddr, uint16_t hostPort,
        string fileDir, 
        int threadNum
    );

    webServer(const webServer&) = delete;
    webServer& operator=(const webServer&) = delete;

    void start();

private:

    // void parseRequest(const string &requestLine);

    // call-back
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&, Buffer*);

    // reponse for GET-method request
    void responseToGet(const string& filePath, const TcpConnectionPtr& conn, bool keepAlive);

    TcpServer server_;
    string fileDir_;    // directory that static-files located in

};