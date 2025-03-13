#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <assert.h>
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop,const std::string& nameArg)
    : baseLoop_(baseloop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0){}

EventLoopThreadPool::~EventLoopThreadPool() {
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_=true;
    for (int i=0; i<numThreads_; ++i) {
        // IO线程名称: 线程池名称 + 线程编号
        auto threadPtr=std::make_unique<EventLoopThread>(cb,name_+std::to_string(i));
        loops_.push_back(threadPtr->startLoop());
        threads_.push_back(std::move(threadPtr));
    }
    // 整个服务端只有一个线程运行baseLoop
    if (numThreads_ == 0 && cb) {
        // 那么不用交给新线程去运行用户回调函数了
        cb(baseLoop_);
    }
}

//轮流获取每一个loop，实现负载均衡
EventLoop* EventLoopThreadPool::getNextLoop() {
    assert(started_);
    // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor 
    // 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    EventLoop* loop = baseLoop_;

    if (!loops_.empty()) {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode){
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    assert(started_);
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1,baseLoop_);
    } else {
        return loops_;
    }
}