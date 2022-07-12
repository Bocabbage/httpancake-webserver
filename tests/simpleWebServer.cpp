#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "Buffer.hpp"
#include <stdio.h>
#include <thread>

void onConnection(const TcpConnectionPtr &conn)
{
    if(conn->connected())
    {
        printf("onConnection(): new connection [%s]\n",
               conn->name().c_str());
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr &conn, Buffer *buf)
{
    // print msg for debugging
    string msg(buf->retrieve_as_string());
    printf("onMessage(): %lu bytes from connection [%s]\n",
           msg.size(), conn->name().c_str());
    printf("msg:\n%s\n", msg.c_str());


    static const string responseLine("HTTP/1.0 200 OK");
    static const string responseHeader("Content-type: text/html\nContent-Length: 49\n\n");
    static const string helloMsg("<html>\n<title>Hi! I'm a message.</title>\n</html>\n");
    
    conn->send(responseLine);
    conn->send(responseHeader);
    conn->send(helloMsg);

}

int main()
{
    printf("main(): pid = %lu\n", gettid());
    EventLoop lp;

    string ipAddr("ANY");
    TcpServer server(&lp, ipAddr, 9981, 5);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    lp.loop();
}