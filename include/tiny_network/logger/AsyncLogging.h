#pragma once
#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "LogFile.h"

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

// AsyncLogging 主要职责：提供大缓冲Large Buffer（默认4MB）存放多条日志消息，缓冲队列BufferVector用于存放多个Large Buffer，
// 为前端线程提供线程安全的写Large Buffer操作；提供专门的后端线程，用于定时或缓冲队列非空时，将缓冲队列中的Large Buffer通过LogFile提供的日志文件操作接口，逐个写到磁盘上。

class AsyncLogging {
public:
    AsyncLogging(const std::string& basename,
                off_t rollSize,
                int flushInterval = 3);
    ~AsyncLogging();

    // 前端调用append写入日志
    void append(const char* logling, int len);

    void start() {
        running_ = true;
        thread_.start();
    }

    void stop() {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    // AsyncLogging数据，按功能分为3部分：
    // 1）维护存放log消息的大缓冲Large Buffer；
    // 2）后端线程；
    // 3）传递给其他类对象的参数，如basename_，rollSize_；
    
    // Large Buffer默认大小4MB，用于存储多条log消息；相对的，还有Small Buffer
    using Buffer = FixedBuffer<kLargeBuffer>;  // Large Buffer Type
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    void threadFunc();
    // 冲刷缓冲数据到文件的超时时间, 默认3秒
    const int flushInterval_;
    // 后端线程是否运行标志
    std::atomic<bool> running_;
    // 日志文件基本名称
    const std::string basename_;
    // 日志文件滚动大小
    const off_t rollSize_;
    // 后端线程
    Thread thread_;
    // 保护前端线程的buffers_
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;  // 当前缓冲
    BufferPtr nextBuffer_;     // 备用空闲缓冲
    BufferVector buffers_;     // 已满缓冲队列
};