#include "Thread.h"
#include "CurrentThread.h"

#include <sstream>
#include <condition_variable>
#include <mutex>

std::atomic_int32_t Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string& name)
    : func_(std::move(func)),
      name_(name){
    setDefaultName();    
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    std::condition_variable cond;
    std::mutex m;
    bool isReady = false;
    started_ = true;
    thread_ = std::make_shared<std::thread>([&]{
        //获取线程的tid值
        tid_ = CurrentThread::tid();
        {
            std::unique_lock<std::mutex> lk(m);
            isReady=true;
        }
        cond.notify_all();
        //在此线程中执行提交的可调用对象
        func_();
    });
    //这里必须等待上面的新线程成功调用tid()获取线程id后，才能继续执行，才能从退出start()函数
    std::unique_lock<std::mutex> lk(m);
    cond.wait(lk,[&](){
        return isReady;
    });
}

void Thread::join() {
    if (thread_->joinable()) {
        thread_->join();
    }
    joined_ = true;
}



void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        std::ostringstream ostr;
        ostr << "Thread " << num;
        name_ = ostr.str();
    }
}