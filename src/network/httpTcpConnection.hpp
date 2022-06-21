#pragma once
#include "TcpConnection.hpp"
#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

enum ProcessState 
{ 
    PARSE_URL = 1,
    PARSE_HEADERS,
    PARSE_BODY,
    RESPONSING,
    PROCESS_SUCCESS,
    PROCESS_FAILED
};

enum RequestType
{ GET_METHOD = 1, POST_METHOD, NULL_METHOD };

// hash-table for mime-type
const unordered_map<string, string> MimeTypes
{
    {".htm", "text/html"},
    {".html", "text/html"},
    {".txt", "text/plain"},
    {".c", "text/plain"},
    {".avi", "video/x-msvideo"},
    {".doc", "application/msword"},
    {".gz", "application/x-gzip"},
    {".bmp", "image/bmp"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".png", "image/png"},
    {".ico", "image/x-icon"},
    {".mp3", "audio/mp3"},
    {"default", "text/html"}
};

class HttpTcpConnection: public TcpConnection
{
public:
    HttpTcpConnection(
        EventLoop* lp, string& name, int sockfd,
        const string &hostAddr, uint16_t hostPort,
        const string &peerAddr, uint16_t peerPort
    );

    ~HttpTcpConnection();

    void setProcessState(ProcessState s) { processState_ = s; }
    void setRequestType(RequestType rt) { requestType_ = rt; }
    void setMimeType(const string& ms) { mimeType_ = ms; }

private:
    // TcpConnection with HTTP parse-state
    ProcessState processState_;
    RequestType requestType_;
    string mimeType_;
};