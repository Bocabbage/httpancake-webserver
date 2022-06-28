#include "Timer.hpp"


std::atomic<int64_t> Timer::s_num_created_;

void Timer::restart(Timestamp now)
{
    if(repeat_)
        expiration_ = add_time(now, interval_);
    else
        expiration_ = Timestamp::invalid();
}