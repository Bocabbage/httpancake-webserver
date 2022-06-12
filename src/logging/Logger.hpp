#pragma once
#include "LogStream.hpp"
#include <string>

using std::string;


class Logger
{
public:
    Logger(const char *fileName, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }

    static void setLogFileName(string fileName) { logFileName_ = fileName; }
    static string getLogFileName() { return logFileName_; }

private:
    class Impl
    {
    public:
        Impl(const char *fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        string basename_;
    };

    Impl impl_;
    static string logFileName_;
};

#define LOG Logger(__FILE__, __LINE__).stream()