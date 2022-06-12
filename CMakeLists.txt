cmake_minimum_required(VERSION 3.16)
project(httpancake)

add_compile_options(-g)

add_subdirectory(src/network networklib)
add_subdirectory(src/reactor reactorlib)
add_subdirectory(src/logging logginglib)
add_subdirectory(tests test-bin)
add_subdirectory(httpancake bin)

