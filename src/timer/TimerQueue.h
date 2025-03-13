#pragma once

#include "noncopyable.h"
#include "Channel.h"
#include "TimerId.h"
#include "Timestamp.h"

#include <functional>
#include <utility>
#include <set>
#include <vector>

class EventLoop;
class Timer;

class TimerQueue : noncopyable {
public:
    using TimerCallback = std::function<void()>;

    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();
    
    //添加一个定时器，
    //Must be thread safe.Usually be called from other threads.
    //此函数只能在EventLoop所属的线程中调用
    void addTimer(TimerCallback cb, Timestamp when, double interval);

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;

    //called when timerfd alarms
    void handleRead();

    //在所属的线程中添加定时器
    void addtimerInLoop(Timer* timer);
    // 移除所有已到期的定时器
    // 1.获取到期的定时器
    // 2.重置这些定时器（销毁或者重复定时任务）
    std::vector<Entry> getExpired(Timestamp now);
    //重置所有已超时的任务
    void reset(const std::vector<Entry>& expired,Timestamp now);
    //插入定时器的内部方法
    bool insert(Timer* timer);

    // FIXME: use unique_ptr<Timer> instead of raw pointers.
    // This requires heterogeneous comparison lookup (N3465) from C++14
    // so that we can find an T* in a set<unique_ptr<T>>.
    //第一个位置上的时间戳存放的是该定时器超时的时刻
    //用户添加的所有的Timer的集合
    TimerList timers_;
    //表明正在获取超时定时器
    bool callingExpiredTimers_;

    //所属的EventLoop
    EventLoop* loop_;
    Channel timerfdChannel_;
    const int timerfd_;
    //一个EventLoop只持有一个TimerQueue对象，
    //而TimerQueue通过std::set持有多个Timer对象，
    //但只会设置一个timerfd和一个对应的Channel,通过一个timerfd来管理多个Timer对象，
    //这个timerfd的超时时间为多个Timer对象中超时时间最近那一个，当timerfd超时超时时，
    //代表至少有一个Timer对象超时，我们就去找到所有超时的Timer对象，并处理
    //因此，实际上，操作系统内部只创建了一个定时器，但我们用这个定时器来实现我们自己的多个Timer
};