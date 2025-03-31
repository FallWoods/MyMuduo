#pragma once

#include "copyable.h"

#include <sys/time.h>
#include <iostream>
#include <string>

//时间戳类，用来表示一个时刻
class Timestamp : public copyable {
public:
    static const int64_t kMicroSecondsPerSecond = 1000 * 1000;
    
    Timestamp();
    Timestamp(int64_t microSecondsSinceEpochArg);
    
    Timestamp(const Timestamp& Ts) = default;
    Timestamp& operator=(const Timestamp& Ts) = default;
    
    void swap(Timestamp& other) {
        int64_t temp = other.microSecondsSinceEpoch_;
        other.microSecondsSinceEpoch_ = microSecondsSinceEpoch_;
        microSecondsSinceEpoch_ = temp;
    }

    // 判断这个时戳间是否有效，大于0有效
    bool valid() const {
        return  microSecondsSinceEpoch_ > 0;
    }
    // 返回一个无效的时间戳
    static Timestamp invalid() {
        return Timestamp();
    }
    // 获取当前时间
    static Timestamp now();

    //将自Epoch时间以来的秒数或微秒数转换成Timestamp
    static Timestamp fromUnixTime(time_t t);
    static Timestamp fromUnixTime(time_t t, int microseconds);

    // 获取时间戳所表示的数值，微秒数
    int64_t microSecondsSinceEpoch() const {
        return microSecondsSinceEpoch_;
    }
    // 秒数
    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    //获取可打印的字符串
    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    bool operator<(const Timestamp& rhs) const {
        return microSecondsSinceEpoch_<rhs.microSecondsSinceEpoch_;
    }
    //加一个时长
    Timestamp& operator+=(double seconds) {
        microSecondsSinceEpoch_ += static_cast<int64_t>(seconds*kMicroSecondsPerSecond);
        return *this;
    }
    //加一个时长
    Timestamp operator+(double seconds) {
        auto res = *this;
        return res += seconds;
    }
private:
    //用来表示时刻，数值为自Epoch时间启经过的微秒数
    int64_t microSecondsSinceEpoch_;
};

//计算2个时间戳的相隔的时间，可正可负,单位为秒
inline double operator-(const Timestamp& lhs, const Timestamp& rhs) {
    int64_t diff = lhs.microSecondsSinceEpoch() - rhs.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}