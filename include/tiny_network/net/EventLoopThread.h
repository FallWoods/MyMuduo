#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread: public noncopyable {
public:
    using ThreadInitCallback = std::function<void (EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name=std::string());
    ~EventLoopThread();

    //启动线程，开始执行loop循环，并返回此线程对应的subloop
    EventLoop* startLoop();
private:
    //线程真正执行的函数，在其中，获取一个线程局部的subloop,并设置为成员loop_,然后调用loop.loop()
    void threadFunc();

    EventLoop* loop_; //GUARDED_BY(mutex_)
    bool exiting_;  //退出标志，为true,表示退出
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};