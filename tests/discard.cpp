#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "Buffer.hpp"
#include <stdio.h>
#include <thread>

void onConnection(const TcpConnectionPtr& conn)
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

void onMessage(const TcpConnectionPtr& conn, Buffer* buf)
{
    string msg(buf->retrieve_as_string());
    printf("onMessage(): %lu bytes from connection [%s]\n",
           msg.size(), conn->name().c_str());
}


int main()
{
    printf("main(): pid = %lu\n", std::this_thread::get_id());
    EventLoop lp;
    string ipAddr("ANY");
    TcpServer server(&lp, ipAddr, 9981, 2);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    lp.loop();
}