#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <atomic>

class Timer : public noncopyable {
public:
    using TimerCallback=std::function<void()>;

    Timer(TimerCallback cb,Timestamp when,double interval);

    //运行时超时回调函数
    void run() const { callback_; }

    //返回超时时刻
    Timestamp expiration() const { return expiration_; }
    //是否是周期的定时器
    bool repeat() const { return repeat_; }
    int64_t sequence()const{ return sequence_; }

    //重启定时器，只对周期Timer有效
    void restart(Timestamp now);

    static int64_t numCreatead() { return s_numCreatead_.load(); }

private:
    const TimerCallback callback_; //超时回调
    Timestamp expiration_;         //超时时刻
    const double interval_;        //周期超时的间隔时间，单位秒；若为0，则说明它是一次性的，不是重复的
    const bool repeat_;            //重复标记，true：周期Timer，false：一次Timer
    const int64_t sequence_;       //全局唯一序列号

    // global increasing number, atomic. help to identify different Timer
    static std::atomic_int64_t s_numCreatead_;
};