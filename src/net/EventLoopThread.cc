#include "EventLoopThread.h"
#include "EventLoop.h"

#include <assert.h>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      callback_(cb){}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) { // not 100% race-free, eg. threadFunc could be running callback_.
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){
    thread_.start();

    EventLoop* loop = nullptr;
    std::unique_lock<std::mutex> lk(mutex_);
    //等待start()函数启动loop结束，此时，loop_不为空
    cond_.wait(lk, [&](){
        return loop_ != nullptr;
    });

    loop = loop_;
    return loop;
}

//此函数即为loop线程的运行函数
void EventLoopThread::threadFunc(){
    EventLoop loop;
    if(callback_){
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lk(mutex_);
        loop_ = &loop;
    }
    cond_.notify_all();
    // 执行EventLoop的loop() 开启了底层的Poller的poll()
    // 这个是subLoop
    loop.loop();
    
    std::lock_guard<std::mutex> lk(mutex_);
    loop_ = nullptr;
}
