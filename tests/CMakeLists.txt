include_directories("../src/network")
include_directories("../src/reactor")
include_directories("../src/logging")

add_executable(nonBlockingLogTest nonBlockingLogTest.cpp)
target_link_libraries(nonBlockingLogTest logging)

add_executable(LoggerTest LoggerTest.cpp)
target_link_libraries(LoggerTest logging)

add_executable(reactorTest1 reactorTest1.cpp)
target_link_libraries(reactorTest1 reactor)

add_executable(threadIdentityTest threadIdentityTest.cpp)
target_link_libraries(threadIdentityTest reactor)

add_executable(discard discard.cpp)
target_link_libraries(discard network)

add_executable(echo echo.cpp)
target_link_libraries(echo network)

add_executable(simpleWebServer simpleWebServer.cpp)
target_link_libraries(simpleWebServer network)

