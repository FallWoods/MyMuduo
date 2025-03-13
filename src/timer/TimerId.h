#pragma once

//TimerId来主要用来作为Timer的唯一标识，用于取消（canceling）Timer
#include "Timer.h"
#include "copyable.h"

class TimerId:public copyable{
friend class TimerQueue;
public:
    TimerId();
    TimerId(Timer*,int64_t);
private:
    Timer* timer_;
    int64_t sequence_;
};