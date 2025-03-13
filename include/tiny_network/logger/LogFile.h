#pragma once

#include "FileUtil.h"

#include <mutex>
#include <memory>

class LogFile {
public:
    LogFile(const std::string& basename,
            off_t rollSize,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char*, int len);
    void flush();
    //滚动日志
    bool rollFile();
private:
    static std::string getLogFileName(const std::string& basename, time_t* now);
    void appendInLock(const char* data, int len);
    // 基础文件名, 用于新log文件命名
    const std::string basename_;
    // 滚动文件大小，每写入rollSize_字节后，就滚动一次
    const off_t rollSize_;
    // 冲刷时间间隔, 默认3 (秒)
    const int flushInterval_;
    // 写数据次数限值, 默认1024
    const int checkEveryN_;
    // 写数据次数计数, 超过限值checkEveryN_时清除, 然后重新计数
    int count_;

    std::unique_ptr<std::mutex> mutex_;
    // 当前log文件创建的起始时间(秒）
    time_t startOfPeriod_;
    // 上次roll日志文件时间(秒)
    time_t lastRoll_;
    // 上次flush日志文件时间(秒)
    time_t lastFlush_;
    // 具体的log文件，每此滚动都会创建新的log文件，向日志文件写入的消息都写入到这个文件中，（可能在此文件的缓冲区中，还没写入到真正的文件中）
    std::unique_ptr<FileUtil> file_;
    // 每天有多少秒
    static const int kRollPerSeconds_ = 60 * 60 * 24;
};