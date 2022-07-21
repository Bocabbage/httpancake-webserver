#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include "FixBuffer.hpp"

class NonBlockingLog
{

using LargeFixBuffer = FixBuffer<largeBufferSize>;

public:
    // singleton
    static NonBlockingLog& getNonBlockingLog(const std::string &logFileName, int flushInterval);
    ~NonBlockingLog();

    NonBlockingLog(const NonBlockingLog&) = delete;
    NonBlockingLog& operator=(const NonBlockingLog&) = delete;

    void stop();
    void append(string&& msg);

private:
    NonBlockingLog() = delete;
    NonBlockingLog(const std::string &logFileName, int flushInterval);

    void threadFunc();
    void writeBack(const string&);

    std::vector<std::unique_ptr<LargeFixBuffer> > buffers_;
    std::unique_ptr<LargeFixBuffer> nowBuffer_;
    std::unique_ptr<LargeFixBuffer> nextBuffer_;
    // LargeFixBuffer* writeBackBuffer_;

    std::string logFileName_;
    int logFileFd_;
    std::thread logThread_;
    std::mutex  mtx_;
    std::condition_variable cond_;
    int flushInterval_;
    bool running_;
};