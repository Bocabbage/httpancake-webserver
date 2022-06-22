#pragma once
#include <string>
#include <unordered_map>
#include "TcpServer.hpp"
#include "httpTcpConnection.hpp"

using std::string;
using std::unordered_map;
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

    // parse function
    void parseRequestStartLine(string &&requestLine, HttpTcpConnection* httpConnPtr);
    void parseHeader(string &&headerLine, unordered_map<string, string>& headerContents);

    // call-back
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&, Buffer*);

    // reponse for GET-method request
    void responseToGet(const string& filePath, const TcpConnectionPtr& conn, bool keepAlive);

    TcpServer server_;
    string fileDir_;    // directory that static-files located in

};