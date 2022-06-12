#include "EventLoop.hpp"
#include "webServer.hpp"
#include <stdio.h>

int main()
{
    // now only for roughly test
    EventLoop lp;
    webServer server(&lp, "127.0.0.1", 9981, "/home/zhangzf/httpancake/httpancake/sourcesForTest", 5);
    printf("mainLoop tid: %d\n", lp.tid());
    server.start();
    lp.loop();
}