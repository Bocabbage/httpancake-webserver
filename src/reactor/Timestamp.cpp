#include "Timestamp.hpp"
#include <stdio.h>
#include <sys/time.h>


Timestamp::Timestamp():
us_since_epoch_(0)
{

}

Timestamp::Timestamp(int64_t microseconds)
  : us_since_epoch_(microseconds)
{
}

std::string Timestamp::to_string() const
{
    char buff[32] = {'\0'};
    int64_t second = us_since_epoch_ / us_per_s;
    int64_t us = us_since_epoch_ % us_per_s;
    snprintf(buff, 31, "%ds %dus", second, us);

    return buff; // std::string(buff);
}

std::string Timestamp::to_formatted_str() const
{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(us_since_epoch_ / us_per_s);
    int microseconds = static_cast<int>(us_since_epoch_ % us_per_s);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
        microseconds);
    return buf;
}

// ----------------- static-function ----------------- //

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);    // syscall
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * us_per_s + tv.tv_usec);
}

Timestamp Timestamp::invalid()
{
    return Timestamp();
}