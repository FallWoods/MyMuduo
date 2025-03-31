#pragma once

#include "noncopyable.h"
#include <functional>
#include <thread>
#include <string>
#include <atomic>
#include <memory>

class Thread : public noncopyable {
public:
    using ThreadFunc = std::function<void ()>;
    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();  // 开启线程
    void join();   // 等待线程汇合
    
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }
    static int numCreated() { return numCreated_.load(); }
private:
    void setDefaultName();  // 设置线程的默认名

    bool started_ = 0; // 是否启动线程
    bool joined_ = 0;  // 是否等待该线程
    pid_t tid_ = 0;    // 线程tid
    std::shared_ptr<std::thread> thread_;
    // 由Thread::start() 调用的回调函数
    // 其实保存的是 EventLoopThread::threadFunc()
    ThreadFunc func_;
    // 线程名字
    std::string name_;
    static std::atomic_int32_t numCreated_;  // 线程索引
};