#pragma once

#include "noncopyable.h"

#include <functional>
#include <memory>
#include <vector>
#include <string>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable{
public:
    using ThreadInitCallback = std::function<void (EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();
    //设置线程数，需在start()之前调用
    void setThreadNum(int numThreads) { numThreads_=numThreads; }
    //启动线程池
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // valid after calling start()
    // round-robin(轮询)subloop
    EventLoop* getNextLoop();

    // with the same hash code, it will always return the same EventLoop
    EventLoop* getLoopForHash(size_t hashCode);

    //获取所有的loops
    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }

    //获取线程池的名称
    const std::string& name() const { return name_; }

private:
    EventLoop* baseLoop_; // 与Acceptor所属EventLoop相同
    std::string name_;    // 线程池名称, 通常由用户指定. 线程池中EventLoopThread名称依赖于线程池名称
    bool started_;        // 线程池是否启动标志
    int numThreads_;      // 线程数
    int next_;            // 新连接到来，所选择的EventLoopThread的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // IO线程列表
    std::vector<EventLoop*> loops_;  // EventLoop列表, 指向的是EventLoopThread线程函数创建的EventLoop对象
};