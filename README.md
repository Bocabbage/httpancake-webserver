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
* [X] HTTP-request parsing & responsing [GET]
* [X] Use Timers to shutdown idle connections
* [ ] MySQL connection-pool

## Test Results

#### **Environment**

* Windows 10 WSL2: Ubuntu22.04
* CPU: Intel Core i5-8265U
* 8GB RAM

#### Test-tools

* [Webbench](https://github.com/linyacool/WebServer/tree/master/WebBench)

#### Configuration

* The Logger-system was disabled
* ThreadPool size: 4

#### Summary

|                            | QPS    |
| -------------------------- | ------ |
| **Short-Connection** | 22,365 |
| **Long-Connection**  | 35,459 |

#### Details

* **Short-connection result**

```shell
./webbench -2 -c 1000 -t 60 http://127.0.0.1:9981/hello.html
```

```shell
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /hello.html HTTP/1.1
User-Agent: WebBench 1.5
Host: 127.0.0.1
Connection: close


Runing info: 1000 clients, running 60 sec.

Speed=1341813 pages/min, 3086356 bytes/sec.
Requests: 1341813 susceed, 0 failed.
```

* **Long-connection result**

```shell
./webbench -k -2 -c 1000 -t 60 http://127.0.0.1:9981/hello.html
```

```shell
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /hello.html HTTP/1.1
User-Agent: WebBench 1.5
Host: 127.0.0.1
Connection: Keep-Alive


Runing info: 1000 clients, running 60 sec.

Speed=2127541 pages/min, 5070639 bytes/sec.
Requests: 2127541 susceed, 0 failed.
```
