set(
    reactor_SRCS
    EventLoop.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    Handler.cpp
    Poller.cpp
    Timer.cpp
    TimerQueue.cpp
    Timestamp.cpp
)

add_library(reactor STATIC ${reactor_SRCS})
target_link_libraries(reactor pthread)
