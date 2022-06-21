include_directories("../src/reactor")
include_directories("../src/network")
include_directories("../src/logging")

set(
    httpancake_SRCS
    httPancake.cpp
    webServer.cpp
)

add_executable(httpancake ${httpancake_SRCS})
target_link_libraries(httpancake reactor network logging)