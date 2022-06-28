// from muduo
#pragma once
#include <set>
#include <vector>
#include "Timestamp.hpp"
#include "Handler.hpp"


class EventLoop;
class Timer;
class TimerId;

class TimerQueue
{
/*
    工作逻辑：每个EventLoop持有一个TimerQueue用来处理定时任务。
    用户直接使用的API是Eventloop提供的run_*系列函数，这系列函数调用TimerQueue
    提供的add_timer接口，将其放入队列中。

    一个TimerQueue持有一个timerfd_，初始化时加入其所属EventLoop的pollfds_中，初始
    时暂不生效。待到有定时任务加入，timerfd_开始工作，以当前队列中最短间隔的任务时间为周期
    触发，在EventLoop::loop()过程中回调初始化时注册函数：TimerQueue::handle_read()，它取出队列
    当前超时的任务，执行回调。

    *对于重复执行的任务（EventLoop::run_every()），每次handler_read()执行完当前所有Timer(s)回调后
    会调用TimerQueue::reset()，它检查出队Timer(s)中repeat_的值，将重复任务重新入队。
*/
using TimerCallback = std::function<void()>;

public:
    TimerQueue(EventLoop* loop);

    // Non-copyable
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

    ~TimerQueue();

    // 提供给 EventLoop 使用的 API
    TimerId add_timer(const TimerCallback &cb,
                      Timestamp when,
                      double interval);
    
    void cancel(TimerId timer_id);

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    void add_timer_in_loop(Timer *timer);
    void handle_read();

    // 核心函数：找出所有超时定时器（塞入vector返回），并从队列中删除
    std::vector<Entry> get_expired(Timestamp now);

    void reset(const std::vector<Entry> &expired, Timestamp now);
    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Handler timerfd_handler;

    // 核心结构：STL二叉搜索树实现
    // 注：缺省情况下std::set采用递增排序
    TimerList timers_;

    // for cancel()
    void cancel_in_loop(TimerId timer_id);
    bool calling_expired_timers_; // atomic
    ActiveTimerSet active_timers_;
    ActiveTimerSet canceling_timers_;


};
