#include "webServer.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"
#include <functional>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>

using std::placeholders::_1;
using std::placeholders::_2;
using std::to_string;

webServer::webServer(
    EventLoop* lp, 
    const string& hostAddr, uint16_t hostPort,
    string fileDir, 
    int threadNum
):
server_(lp, hostAddr, hostPort, threadNum, HTTPTCPCONN),
fileDir_(fileDir)
{
    server_.setConnectionCallback(std::bind(&webServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&webServer::onMessage, this, _1, _2));
}

void webServer::start()
{
    // call only once
    server_.start();
}

void webServer::onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        // printf("onConnection(): new connection [%s]\n",
        //        conn->name().c_str());
        LOG << "onConnection(): new connection [" << conn->name().c_str() << "]";
    }
    else
    {
        // printf("onConnection(): connection [%s] is down; now %d conns.\n",
        //        conn->name().c_str(), server_.connSize());

        LOG << "onConnection(): connection [" << conn->name().c_str() << "] is down; "
            << "now " << server_.connSize() << "conns.";
        
    }
}

void webServer::onMessage(const TcpConnectionPtr& conn, Buffer *inBuffer)
{
    string requestMsg(inBuffer->retrieve_as_string());
    size_t requestLineEndIdx = requestMsg.find('\n');
    // without complete requestLine
    if(requestLineEndIdx == string::npos)
    {
        inBuffer->append(requestMsg);
        return;
    }
    
    string requestLine = requestMsg.substr(0, requestLineEndIdx);


    size_t firstSpaceIdx = requestLine.find(' ');
    string method = requestLine.substr(0, firstSpaceIdx);
    // 1. parse the request-method
    if(method == "GET") 
    {
        // 2. parse the request-obj
        size_t secondSpaceIdx = requestLine.find(' ', firstSpaceIdx + 1);
        string filePath = fileDir_ + requestLine.substr(firstSpaceIdx + 1, secondSpaceIdx - firstSpaceIdx - 1);

        bool keepAlive = false;
        // DIRTY: wait to be optimized
        if(requestMsg.find("Keep-Alive") != string::npos || requestMsg.find("keep-alive") != string::npos)
            keepAlive = true;

        // 3. construct the response
        responseToGet(filePath, conn, keepAlive);
    }
    
}

void webServer::responseToGet(const string& filePath, const TcpConnectionPtr& conn, bool keepAlive=false)
{
    // for debug
    std::cout << "Request filePath: " << filePath << std::endl;

    struct stat fstat;
    bzero(&fstat, sizeof(fstat));
    if(lstat(filePath.c_str(), &fstat) < 0 && errno == ENOENT)
    {
        std::cout << "Find Source file failed" << std::endl;

        // source not exist
        static const string notFoundResponseLine("HTTP/1.0 404 Not Found\n");
        static const string notFoundResponseHeader("Content-type: text/html\nContent-Length: 44\n\n");
        static const string notFoundMsg("<html>\nNo such file.\n</html>\n");

        conn->send(notFoundResponseLine + notFoundResponseHeader + notFoundMsg);
    }
    else
    {
        size_t fileSz = fstat.st_size;

        std::cout << conn->name() << " : Find Source file, file-sz: " << fileSz << std::endl;

        static const string successResponseLine("HTTP/1.0 200 OK\n");
        string successResponseHeader("Content-type: text/html\nContent-Length: ");
        successResponseHeader += to_string(fileSz);
        // HTTP1.0 + keep-alive option
        if(keepAlive)
            successResponseHeader += "\nConnection: keep-alive";
        successResponseHeader += "\n\n";

        // read the file into mem
        int requestFd = open(filePath.c_str(), O_RDONLY);
        void *fileMap = mmap(
            NULL, fileSz, PROT_READ, MAP_PRIVATE, requestFd, 0
        );

        string successResponseMsg((char *)fileMap, fileSz);

        munmap(fileMap, fileSz);

        // send response msg
        conn->send(successResponseLine + successResponseHeader + successResponseMsg);
    }
}