#define __STDC_LIMIT_MACROS  // 允许程序使用C99：<stdint.h>中定义的宏
#include "TimerQueue.hpp"
#include "EventLoop.hpp"
#include "Timer.hpp"
#include "TimerId.hpp"

#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h> // for 'read()' function
#include <assert.h>

namespace detail
{
int create_timefd()
{   
    // timerfd: 通过描述符可读传递超时通知

    // timerfd_create(int clockid, int flags);
    // @ clockid: 指定定时器类型
    //      CLOCK_MONOTONIC: 以固定的速率运行，从不进行调整和复位 ,它不受任何系统time-of-day时钟修改的影响
    //      CLOCK_REALTIME : Systemwide realtime clock. 系统范围内的实时时钟
    // @ flags:
    //      0           : 不设置
    //      TFD_NONBLOCK: 设置为非阻塞
    //      TFD_CLOEXEC : 同O_CLOEXEC，当执行exec()函数时自动关闭当前描述符（关系到父子进程的文件描述符继承问题）
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    
    return timerfd;
}

struct timespec how_much_time_from_now(Timestamp when)
{
    int64_t us = when.us_since_epoch()
                 - Timestamp::now().us_since_epoch();

    if(us < 100)
        us = 100;

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(us / Timestamp::us_per_s);
    ts.tv_nsec = static_cast<long>((us % Timestamp::us_per_s) * 1000);
    return ts;
}

void read_timerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    // read将返回超时次数；设置为NONBLOCK时，若未有超时，返回EAGAIN
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
}

void reset_timerfd(int timerfd, Timestamp expiration)
{
    // struct itimerspec {
    //            struct timespec it_interval;  /* Interval for periodic timer */
    //            struct timespec it_value;     /* Initial expiration */
    //        };
    struct itimerspec new_value;
    struct itimerspec old_value;

    bzero(&new_value, sizeof(new_value));
    bzero(&old_value, sizeof(old_value));
    // 用于设置定时器超时时间（expiration）：距离现在（now）expiration的时间
    new_value.it_value = how_much_time_from_now(expiration); 

    // int timerfd_settime(int tfd, int flags, const struct itimerspec *newValue, struct itimerspec *oldValue)
    // @flags   : 0 表示相对时间，1 表示绝对时间
    // @newValue: 指定新的超时时间。该时间大于0则启动定时器，否则关闭
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);

}

}

using namespace detail;

TimerQueue::TimerQueue(EventLoop *lp):
loop_(lp), timerfd_(create_timefd()), 
timerfd_handler(timerfd_, loop_), timers_()
{
    // 设置队列timefd_的回调：handle_read，该回调函数会调用每个超时Timer(s)内的回调
    // ! 这步设置也会将timerfd_交给持有计时器队列的Eventloop，调用update_handler将其加入poll列表中
    timerfd_handler.setReadCallback(std::bind(&TimerQueue::handle_read, this));
    timerfd_handler.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);
    for(TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
        delete it->second;
}

TimerId TimerQueue::add_timer(const TimerCallback &cb,
                              Timestamp when,
                              double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    // 通过这一间接调用，使add_timer成为线程安全的
    loop_->runInLoop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
    return TimerId(timer);
}

void TimerQueue::add_timer_in_loop(Timer *timer)
{
    // loop_->assert_in_loopthread();
    bool earliest_changed = insert(timer);

    if(earliest_changed)
    {
        // 如果插入的新定时器任务是目前最早需要完成的（或队列为空），
        // 则重设整个队列的超时响应信号时间（若为空，则此次调用也首次启动timefd_）
        reset_timerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::handle_read()
{
    // loop_->assert_in_loopthread();
    Timestamp now(Timestamp::now());
    read_timerfd(timerfd_, now);

    std::vector<Entry> expired = get_expired(now);

    for(auto it = expired.begin(); it != expired.end(); ++it)
        it->second->run();

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::get_expired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    auto it = timers_.lower_bound(sentry);  // 找出已超时的定时器
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    // Add in 8.11
    for(auto &entry: expired)
    {
        ActiveTimer timer(entry.second, entry.second->sequence());
        size_t n = active_timers_.erase(timer);
        assert(n == 1); // (void)n;
    }

    assert(timers_.size() == active_timers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp next_expire;
    for(auto it = expired.begin(); it != expired.end(); ++it)
    {
        if(it->second->repeat())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            delete it->second;
        }
    }

    if(!timers_.empty())
        next_expire = timers_.begin()->second->expiration();
    
    if(next_expire.valid())
        reset_timerfd(timerfd_, next_expire);
}

bool TimerQueue::insert(Timer *timer)
{
    // @earliest_changed: 
    // 当前要插入的定时器是否已为队列中最早需要完成任务的
    bool earliest_changed = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first)
        earliest_changed = true;
    
    std::pair<TimerList::iterator, bool> result = 
        timers_.insert(std::make_pair(when, timer));
    assert(result.second);
    return earliest_changed;
}

void TimerQueue::cancel(TimerId timer_id)
{
    loop_->runInLoop(
        std::bind(&TimerQueue::cancel_in_loop, this, timer_id)
    );
}

void TimerQueue::cancel_in_loop(TimerId timer_id)
{
    // loop_->assert_in_loopthread();
    assert(timers_.size() == active_timers_.size());
    ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
    ActiveTimerSet::iterator it = active_timers_.find(timer);
    if(it != active_timers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); // (void)n;
        delete it->first;
        active_timers_.erase(it);
    }
    else if(calling_expired_timers_)
    {
        canceling_timers_.insert(timer);
    }
    assert(timers_.size() == active_timers_.size());
}