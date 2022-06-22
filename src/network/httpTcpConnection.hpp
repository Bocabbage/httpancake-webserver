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
    PROCESS_FAILED,
// Error-state
    REQUEST_LINE_ERROR
};

enum RequestType
{ GET_METHOD = 1, POST_METHOD, NULL_METHOD };

enum HttpVersion
{ HTTP1_0 = 1, HTTP1_1 };

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
    void setHttpVersion(HttpVersion hv) { httpVersion_ = hv; }
    void setURL(const string& url) { URL_ = url; }
    void setKeepAlive(bool ka) { keepAlive_ = ka; }

    ProcessState state() const { return processState_; }
    RequestType requestType() const { return requestType_; }
    string mimeType() const { return mimeType_; }
    HttpVersion httpVersion() const { return httpVersion_; }
    string url() const { return URL_; }
    bool keepAlive() const { return keepAlive_; }

private:
    // TcpConnection with HTTP parse-state
    ProcessState processState_;
    RequestType requestType_;
    string mimeType_;
    HttpVersion httpVersion_;
    string URL_;
    bool keepAlive_;
};