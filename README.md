# WebServer(Httpancake): self-learning project

## Introduction

A simple C++ webserver for responsing HTTP-request(s). It's a self-learning project which draws heavily on the design of the following projects:

* [muduo](https://github.com/chenshuo/muduo)
* [WebServer](https://github.com/linyacool/WebServer)

Different from these projects, this webserver is implemented based on C++11 standard library (e.g., std::thread, std::mutex).

## Environment

* Ubuntu 20.04
* gcc 9.3.0 [g++]


## Usage

**【STILL DIRTY NOW】**  Hard-coding the server-address-port and source path, make it and run:

```shell
./build/bin/httpancake
```



## Technical Points

* [ ] Configuration interface
* [X] Reactor server model
* [X] EventLoop threadpool [One-loop-per-thread]
* [X] Simple async log system
* [ ] HTTP-request parsing & responsing [GET, unfinished: POST]
* [X] Use Timers to shutdown idle connections
* [ ] MySQL connection-pool


## Test Results (Webbench)

**【UNFINISHED】**
