#include "Logger.hpp"
#include "NonBlockingLog.hpp"
#include <pthread.h>
#include <memory>

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static std::unique_ptr<NonBlockingLog> nbLogger_;

string Logger::logFileName_ = "./httpancake.log";

void once_init()
{
    nbLogger_.reset(new NonBlockingLog(Logger::getLogFileName(), 3));
}

void output(string&& msg)
{
    pthread_once(&once_control_, once_init);
    nbLogger_->append(std::move(msg));
}

Logger::Impl::Impl(const char *fileName, int line):
    stream_(),
    line_(line),
    basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime()
{

}

Logger::Logger(const char *fileName, int line):
    impl_(fileName, line)
{  }

Logger::~Logger()
{
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    LogStream::SmallFixBuffer& buff(stream().buffer());
    output(buff.getContent());
}