#include "Timestamp.h"

#include <inttypes.h>

// PRId64, for printf data in cross platform

//TODO:使用chrono库重构

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpochArg)
    : microSecondsSinceEpoch_(microSecondsSinceEpochArg){}

Timestamp Timestamp::now() {
    int res = -1;
    struct timeval tv;
    while(res < 0){
        res = gettimeofday(&tv,0);
    }
    //获取的是UTC时间，要加上8小时，转换成北京时间
    return Timestamp(tv.tv_sec * kMicroSecondsPerSecond + tv.tv_usec + 8 * 60 * 60 * kMicroSecondsPerSecond);
}

Timestamp Timestamp::fromUnixTime(time_t t, int microseconds) {
    return Timestamp(static_cast<int64_t>(t * kMicroSecondsPerSecond) + microseconds);
}

Timestamp Timestamp::fromUnixTime(time_t t) {
    return fromUnixTime(t, 0);
}

std::string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf,sizeof(buf),"%" PRId64 ".%06" PRId64 "",seconds,microseconds);
    return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    if (showMicroseconds) {
        int microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
        snprintf(buf,sizeof(buf),"%4d/%02d/%02d %02d:%02d:%02d.%06d ",
                tm_time.tm_year + 1900,
                tm_time.tm_mon + 1,
                tm_time.tm_mday,
                tm_time.tm_hour,
                tm_time.tm_min,
                tm_time.tm_sec,
                microseconds);
    } else {
        snprintf(buf,sizeof(buf),"%4d%02d%02d %02d:%02d:%02d ",
                tm_time.tm_year + 1900,
                tm_time.tm_mon + 1,
                tm_time.tm_mday,
                tm_time.tm_hour,
                tm_time.tm_min,
                tm_time.tm_sec);
    }
    return buf;
}