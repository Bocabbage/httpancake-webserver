include_directories("../reactor")

set(
    network_SRCS
    Acceptor.cpp
    Buffer.cpp
    Socket.cpp
    socketUtils.cpp
    TcpConnection.cpp
    httpTcpConnection.cpp
    TcpServer.cpp
)

add_library(network STATIC ${network_SRCS})
target_link_libraries(network reactor)