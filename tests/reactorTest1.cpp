#include "EventLoop.hpp"
#include "Handler.hpp"
#include <unistd.h>
#include <strings.h>
#include <sys/timerfd.h>

EventLoop* gLoop;

void timeout()
{
    printf("Timeout!\n");
    gLoop->stopLoop();
}

int main()
{
    EventLoop loop;
    gLoop = &loop;

    int timefd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Handler handler(timefd, &loop);
    handler.setReadCallback(timeout);
    handler.enableReading();

    struct itimerspec howlong;
    ::bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timefd, 0, &howlong, NULL);

    printf("loopStart!\n");
    loop.loop();

    ::close(timefd);

}