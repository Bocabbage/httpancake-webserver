#include "webServer.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"
#include <functional>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
#include <cctype> // std::tolower
#include <algorithm>
#include <regex>

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
        LOG << "onConnection(): new connection [" << conn->name().c_str() << "]";
    }
    else
    {
        LOG << "onConnection(): connection [" << conn->name().c_str() << "] is down; "
            << "now " << server_.connSize() << "conns.";
    }
}

void webServer::onMessage(const TcpConnectionPtr& conn, Buffer *inBuffer)
{
    // Remove the const: DIRTY!
    HttpTcpConnection* httpConnPtr = static_cast<HttpTcpConnection*>(conn.get());

    /* state-check */
    // 1. URL-parsing
    if(httpConnPtr->state() == PARSE_URL)
    {
       
        string requestLine = inBuffer->retrieve_line_as_string();
        if(requestLine.empty())
        {
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);
        }
        else
        {
            parseRequestStartLine(std::move(requestLine), httpConnPtr);
        }

        if(httpConnPtr->state() == REQUEST_LINE_ERROR)
        {
            LOG << "Connection Error: " << conn->name().c_str() << "\tREQUEST-LINE-ERROR";
            httpConnPtr->setProcessState(PROCESS_FAILED);
        }
    }

    // 2. Header-parsing
    if(httpConnPtr->state() == PARSE_HEADERS)
    {
        // Current version: ignore the whole header information except the 'keep-alive'
        unordered_map<string, string> headerContents;

        string headerLine = inBuffer->retrieve_line_as_string();
        while(!headerLine.empty())
        {
            parseHeader(std::move(headerLine), headerContents);
            headerLine = inBuffer->retrieve_line_as_string();
        }

        // keepAlive setting
        if(headerContents.find("connection") != headerContents.end())
        {
            std::regex keepAliveRegex("keep-alive/i");
            httpConnPtr->setKeepAlive(
                std::regex_match(headerContents["connection"], keepAliveRegex)
            );
        }

        httpConnPtr->setProcessState(PARSE_BODY);
    }

    // 3. Body-parsing
    if(httpConnPtr->state() == PARSE_BODY)
    {

    }

    // 4. responsing
    if(httpConnPtr->state() == RESPONSING)
    {

    }

    // 5. success-or-failed
    if(httpConnPtr->state() == PROCESS_SUCCESS)
    {

    }
    else if(httpConnPtr->state() == PROCESS_FAILED)
    {

    }


    // string requestMsg(inBuffer->retrieve_as_string());
    // size_t requestLineEndIdx = requestMsg.find('\n');
    // without complete requestLine
    // if(requestLineEndIdx == string::npos)
    // {
    //     inBuffer->append(requestMsg);
    //     return;
    // }
    
    // string requestLine = requestMsg.substr(0, requestLineEndIdx);


    // size_t firstSpaceIdx = requestLine.find(' ');
    // string method = requestLine.substr(0, firstSpaceIdx);
    // // 1. parse the request-method
    // if(method == "GET") 
    // {
    //     // 2. parse the request-obj
    //     size_t secondSpaceIdx = requestLine.find(' ', firstSpaceIdx + 1);
    //     string filePath = fileDir_ + requestLine.substr(firstSpaceIdx + 1, secondSpaceIdx - firstSpaceIdx - 1);

    //     bool keepAlive = false;
    //     // DIRTY: wait to be optimized
    //     if(requestMsg.find("Keep-Alive") != string::npos || requestMsg.find("keep-alive") != string::npos)
    //         keepAlive = true;

    //     // 3. construct the response
    //     responseToGet(filePath, conn, keepAlive);
    // }
    
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

void webServer::parseRequestStartLine(string &&requestLine, HttpTcpConnection* httpConnPtr)
{
    do
    {
        // Parse request-method
        if(requestLine.find("GET"))
            httpConnPtr->setRequestType(GET_METHOD);
        else if(requestLine.find("POST"))
            httpConnPtr->setRequestType(POST_METHOD);
        else
        {
            httpConnPtr->setRequestType(NULL_METHOD);
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);
            break;
        }
            
        // Parse URL
        size_t firstSpaceIdx = requestLine.find(' ');
        if(firstSpaceIdx == string::npos)
        {
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);
            break;
        }

        size_t secondSpaceIdx = requestLine.find(' ', firstSpaceIdx + 1);
        if(secondSpaceIdx == string::npos)
        {
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);
            break;
        }

        httpConnPtr->setURL(fileDir_ + requestLine.substr(firstSpaceIdx + 1, secondSpaceIdx - firstSpaceIdx - 1));


        // Parse HTTP-type
        if(requestLine.find("HTTP/1.1"))
            httpConnPtr->setHttpVersion(HTTP1_1);
        else if(requestLine.find("HTTP/1.0"))
            httpConnPtr->setHttpVersion(HTTP1_0);
        else
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);

    } while (false);

}

void parseHeader(string &&headerLine, unordered_map<string, string>& headerContents)
{
    size_t keyValDivIdx = headerLine.find(':');
    // silently return if the headerLine has no ':'
    if(keyValDivIdx == string::npos)
        return;
    
    // All the keys are transformed into lower-case
    string key = headerLine.substr(0, keyValDivIdx);
    std::transform(
        key.begin(), key.end(), key.begin(), 
        [](unsigned char c){ return std::tolower(c); }
    );

    string val = headerLine.substr(keyValDivIdx+1, key.length() - keyValDivIdx - 1);

    headerContents.insert({key, val});
}