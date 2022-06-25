#include "EventLoop.hpp"
#include "webServer.hpp"
#include "Logger.hpp"
#include <stdio.h>

int main()
{
    // now only for roughly test
    Logger::setLogFileName("./httPancake.log");
    EventLoop lp;
    webServer server(&lp, "127.0.0.1", 9981, "/home/zhangzf/httpancake/httpancake/sourcesForTest", 5);
    server.start();
    lp.loop();
}