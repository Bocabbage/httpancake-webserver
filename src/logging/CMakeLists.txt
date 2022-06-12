set(
    logging_SRCS
    LogStream.cpp
    Logger.cpp
    NonBlockingLog.cpp
)

add_library(logging STATIC ${logging_SRCS})
target_link_libraries(logging pthread)
