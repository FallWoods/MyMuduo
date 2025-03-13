#include "Timer.h"

std::atomic_int64_t Timer::s_numCreatead_(0);

Timer::Timer(TimerCallback cb,Timestamp when,double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numCreatead_++) {}

//如果此定时器是重复的，则重启它，否则将它设置为失效状态
void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = now + interval_;
    } else {
        expiration_ = Timestamp::invalid();
    }
}