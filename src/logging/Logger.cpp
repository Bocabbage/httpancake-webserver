#include "Logger.hpp"
#include "NonBlockingLog.hpp"
#include <pthread.h>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>
using time_point = std::chrono::system_clock::time_point;

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static std::unique_ptr<NonBlockingLog> nbLogger_;

string Logger::logFileName_ = "./httpancake.log";

string serializeTimePoint( const time_point& time, const string& format)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::gmtime(&tt); //GMT (UTC)
    //std::tm tm = *std::localtime(&tt); //Locale time-zone, usually UTC by default.
    std::stringstream ss;
    ss << std::put_time( &tm, format.c_str() );
    return ss.str();
}

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
    auto timePoint = std::chrono::system_clock::now();
    stream_ << '[' <<  serializeTimePoint(timePoint, "UTC: %Y-%m-%d %H:%M:%S") << ']';
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