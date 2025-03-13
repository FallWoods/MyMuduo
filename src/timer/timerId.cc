#include "TimerId.h"

TimerId::TimerId()
    : timer_(nullptr),
      sequence_(0) {}

TimerId::TimerId(Timer* timer,int64_t seq)
    : timer_(timer),
      sequence_(seq) {}