#include "EventLoop.hpp"
#include "webServer.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"
#include <functional>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
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
    int threadNum,
    int expiredSec
):
server_(lp, hostAddr, hostPort, threadNum, HTTPTCPCONN),
fileDir_(fileDir),
expiredSec_(expiredSec)
{
    server_.setConnectionCallback(std::bind(&webServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&webServer::onMessage, this, _1, _2));

    // shutdown idle connections
    if(expiredSec > 0)
    {
        lp->runEvery(1.0, std::bind(&webServer::onTimer, this));
        connBuckets_.resize(expiredSec_);
    }

}

void webServer::start()
{
    // call only once
    server_.start();
}

void webServer::onTimer()
{
    // LOG << "webServer::onTimer called";
    connBuckets_.push_back(Bucket());
}

void webServer::onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        LOG << "onConnection(): new connection [" << conn->name().c_str() << "]";
        // For timing
        if(expiredSec_ > 0)
        {
            std::shared_ptr<Entry> entryPtr(std::make_shared<Entry>(conn));
            connBuckets_.back().insert(entryPtr);
            std::weak_ptr<Entry> weakEntryPtr(entryPtr);
            conn->setContext(weakEntryPtr);
        }   
    }
    else
    {
        LOG << "onConnection(): connection [" << conn->name().c_str() << "] is down; "
            << "now " << server_.connSize() << "conns.";
    }
}

void webServer::onMessage(const TcpConnectionPtr& conn, Buffer *inBuffer)
{
    if(expiredSec_ > 0)
    {
        assert(!conn->getContext().empty());
        std::weak_ptr<Entry> weakEntryPtr(boost::any_cast<std::weak_ptr<Entry> >(conn->getContext()));
        std::shared_ptr<Entry> entryPtr(weakEntryPtr.lock());
        if(entryPtr)
            connBuckets_.back().insert(entryPtr);
    }

    // Remove the const: DIRTY!
    // upcasting: use dynamic-cast
    HttpTcpConnection* httpConnPtr = dynamic_cast<HttpTcpConnection*>(conn.get());

    do
    {
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
            else
                httpConnPtr->setProcessState(PARSE_HEADERS);
        }

        // 2. Header-parsing
        if(httpConnPtr->state() == PARSE_HEADERS)
        {
            // Current version: ignore the whole header information except the 'keep-alive'
            unordered_map<string, string> headerContents;

            string headerLine = inBuffer->retrieve_line_as_string();
            while(!headerLine.empty() && headerLine[0] != '\r')
            {
                if(headerLine.back() == '\r')
                    headerLine.pop_back();
                parseHeader(std::move(headerLine), headerContents);
                headerLine = inBuffer->retrieve_line_as_string();
            }

            // keepAlive setting
            if(httpConnPtr->httpVersion() == HTTP1_1)
                httpConnPtr->setKeepAlive(true);    // default-on for http1.1

            if(headerContents.find("connection") != headerContents.end())
            {
                if(httpConnPtr->httpVersion() == HTTP1_1)
                {
                    std::regex closeRegex("close", std::regex::icase);
                    httpConnPtr->setKeepAlive(  
                        !std::regex_match(headerContents["connection"], closeRegex)
                    );
                }
                else
                {
                    std::regex keepAliveRegex("keep-alive", std::regex::icase);
                    httpConnPtr->setKeepAlive(  
                        std::regex_match(headerContents["connection"], keepAliveRegex)
                    );
                }
            }

            httpConnPtr->setProcessState(PARSE_BODY);
        }

        // 3. Body-parsing
        if(httpConnPtr->state() == PARSE_BODY)
        {
            // Current version: ignore the whole body
            // Should be implemented for 'POST-method' later
            string headerLine = inBuffer->peek_line();
            while(!headerLine.empty())
            {
                // DIRTY!
                if(headerLine.find("HTTP/") != string::npos)
                    break;
                else
                    inBuffer->retrieve(headerLine.size());
            }

            httpConnPtr->setProcessState(RESPONSING);
        }

        // 4. responsing
        if(httpConnPtr->state() == RESPONSING)
        {
            switch(httpConnPtr->requestType())
            {
                case GET_METHOD:
                    responseToGet(httpConnPtr);
                    break;
                
                case POST_METHOD:
                    responseToPost(httpConnPtr);
                    break;
            }

        }

        // 5. success-or-failed
        bool keepAliveState = httpConnPtr->keepAlive();
        if(httpConnPtr->state() == PROCESS_SUCCESS)
        {
            if(httpConnPtr->requestType() == GET_METHOD)
                LOG << "onMessage(): [" << conn->name().c_str() << "] get: " << httpConnPtr->url() << " successfully.";     
        }
        else if(httpConnPtr->state() == PROCESS_FAILED)
        {
            LOG << "onMessage(): [" << conn->name().c_str() << "] request: " << httpConnPtr->url() << " failed.";
        }
        httpConnPtr->clearState();
        if(!keepAliveState)
            conn->forceClose();
    }while (inBuffer->readable_bytes() != 0);
}

void webServer::parseRequestStartLine(string &&requestLine, HttpTcpConnection* httpConnPtr)
{
    do
    {
        // Parse request-method
        if(requestLine.find("GET") != string::npos)
            httpConnPtr->setRequestType(GET_METHOD);
        else if(requestLine.find("POST") != string::npos)
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

        string fileURL = fileDir_ + requestLine.substr(firstSpaceIdx + 1, secondSpaceIdx - firstSpaceIdx - 1);
        size_t suffixIdx = fileURL.find_last_of('.');
        httpConnPtr->setMimeType(fileURL.substr(suffixIdx, fileURL.length() - suffixIdx));
        httpConnPtr->setURL(std::move(fileURL));

        // Parse HTTP-type
        if(requestLine.find("HTTP/1.1"))
            httpConnPtr->setHttpVersion(HTTP1_1);
        else if(requestLine.find("HTTP/1.0"))
            httpConnPtr->setHttpVersion(HTTP1_0);
        else
            httpConnPtr->setProcessState(REQUEST_LINE_ERROR);

    } while (false);

}

void webServer::parseHeader(string &&headerLine, unordered_map<string, string>& headerContents)
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

    // keyValDivIdx + 2: ignore ': '
    string val = headerLine.substr(keyValDivIdx+2, key.length() - keyValDivIdx - 1);

    headerContents.insert({key, val});
}

void webServer::responseToGet(HttpTcpConnection* httpConnPtr)
{
    string filePath = httpConnPtr->url();
    struct stat fstat;
    bzero(&fstat, sizeof(fstat));
    if(lstat(filePath.c_str(), &fstat) < 0 && errno == ENOENT)
    {
        std::cout << "Find Source file failed" << std::endl;

        // source not exist
        static const string notFoundResponseLine(" 404 Not Found\n");
        static const string notFoundResponseHeader("Content-type: text/html\nContent-Length: 44\n\n");
        static const string notFoundMsg("<html>\nNo such file.\n</html>\n");

        string httpVer = httpConnPtr->httpVersion() == HTTP1_1 ? "HTTP/1.1" : "HTTP/1.0";

        httpConnPtr->send(httpVer + notFoundResponseLine + notFoundResponseHeader + notFoundMsg);
        httpConnPtr->setProcessState(PROCESS_FAILED);
    }
    else
    {
        size_t fileSz = fstat.st_size;

        static const string successResponseLine("HTTP/1.0 200 OK\n");
        string successResponseHeader("Content-type: ");
        successResponseHeader += httpConnPtr->mimeType();
        successResponseHeader += "\nContent-Length: ";
        successResponseHeader += to_string(fileSz);
        // HTTP1.0 + keep-alive option
        if(httpConnPtr->keepAlive())
            successResponseHeader += "\nConnection: keep-alive";
        else
            successResponseHeader += "\nConnection: close";
        successResponseHeader += "\n\n";

        // read the file into mem
        int requestFd = open(filePath.c_str(), O_RDONLY);
        void *fileMap = mmap(
            NULL, fileSz, PROT_READ, MAP_SHARED, requestFd, 0
        );
        close(requestFd);

        char *fileMapAddr = static_cast<char*>(fileMap);
        // fixme: can be more robust for large-file sending
        string successResponseMsg(fileMapAddr, fileSz);

        munmap(fileMap, fileSz);

        // send response msg
        httpConnPtr->send(successResponseLine + successResponseHeader + successResponseMsg);
        httpConnPtr->setProcessState(PROCESS_SUCCESS);
    }
}

void webServer::responseToPost(HttpTcpConnection* httpConnPtr)
{

}