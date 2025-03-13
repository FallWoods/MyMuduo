#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "Timestamp.h"
#include "TimerQueue.h"
#include "CurrentThread.h"

#include <assert.h>
#include <algorithm>
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>

// 防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread=nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 50000;

//TODO:eventfd使用
int createEventfd() {
    int evfd = ::eventfd(0,  EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0) {
        //log
        std::cout<< "eventfd error: " << errno <<std::endl;
    }
    return evfd;
}

EventLoop* getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : threadId_(CurrentThread::tid()),
      looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(std::make_unique<TimerQueue>(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this,wakeupFd_)),
      currentActiveChannel_(nullptr),
      iteration_(0){
    //日志操作
    std::cout << "EventLoop created " << this << " the index is " << threadId_ <<std::endl;
    std::cout << "EventLoop created wakeupFd " << wakeupChannel_->fd() <<std::endl;
    if (t_loopInThisThread) {
        //当前线程的EventLoop对象不为空，说明重复创建了
        //日志操作
        std::cout << "Another EventLoop" << t_loopInThisThread << " exists in this thread " << threadId_  <<std::endl;
    } else {
        //当前线程尚未包含EventLoop对象
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    // channel移除所有感兴趣事件
    wakeupChannel_->disableAll();
    // 将channel从EventLoop中删除
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// void EventLoop::assertInLoopThread(){
//     if(!isInLoopThread()){
//         abortNotInLoopThread();
//     }
// }

// void EventLoop::abortNotInLoopThread(){
//     //log
//     //LOG_FATAL终止线程
// }

void EventLoop::loop() {
    assert(!looping_);  //避免重复的loop

    looping_=true;
    quit_=false; // FIXME: what if someone calls quit() before loop() ?
    while (!quit_) {
        activeChannels_.clear(); //清空激活通道列表
        //开始轮询
        std::cout<< "have polled: "<< iteration_ <<std::endl;
        pollReturnTime_ = poller_->poll(kPollTimeMs,activeChannels_);
        ++iteration_; //轮询次数加1
        //log
        
        //处理所有激活事件
        //TODO：sort channel by priority queue
        for (Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            //通过Channel::handleEvent回调处理事件
            channel->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO thread：mainLoop accept fd 打包成 chennel 分发给 subLoop
         * mainLoop实现注册一个回调，交给subLoop来执行，wakeup subLoop 之后，让其执行注册的回调操作
         * 这些回调函数在 std::vector<Functor> pendingFunctors_; 之中
         */
        doPendingFunctors();
    }
    //log
    printf("EventLoop stop looping\n");
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    /**
     * TODO:生产者消费者队列派发方式和muduo的派发方式
     * 有可能是别的线程调用quit(调用线程不是生成EventLoop对象的那个线程)
     * 比如在工作线程(subLoop)中调用了IO线程(mainLoop)
     * 这种情况会唤醒主线程
     */
    //???
    // There is a chance that loop() just executes while(!quit_) and exits,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (isInLoopThread()) {
        wakeup();
    }
}

//使用poller对象的updateChannel()，来更新poller对象监听的通道
//包括暂停监听、重新监听、对一个新的channel进行监听
//必须在channel所属的loop线程运行
void EventLoop::updateChannel(Channel* channel){
    // assert(channel->ownerloop()==this);
    poller_->updateChannel(channel);
}

//使用具体poller对象的removeChannel(),来从其中删除只当channel
//必须在channel所属的loop线程运行
void EventLoop::removeChannel(Channel* channel) {
    // if(eventHandling_){
    //     //如果此时正在处理激活事件，则此channel要么正在被处理，既currentActiveChannel_==channel,
    //     //此时，将channel移除，不会影响后续处理
    //     //要么不在激活的channel序列中，也可以直接移除
    //     //这两种情况下，可以将channel移除
    //     assert(channel==currentActiveChannel_ || 
    //     std::find(activeChannels_.begin(),activeChannels_.end(),channel)==activeChannels_.end());
    // }
    poller_->removeChannel(channel);
}

//判断poller的channels_中是否保存着channel
bool EventLoop::hasChannel(Channel* channel){
    // assert(channel->ownerloop()==this);
    // assertInLoopThread();
    return poller_->hasChannel(channel);
}


// //取消指定的定时器
// //调用此函数的不必是loop线程
// void EventLoop::cancel(TimerId timerId){
//     timerQueue_->cancel(timerId);
// }

/**
* 执行用户任务
* @param cb 用户任务函数
* @note 可以被多个线程执行：
* 如果当前线程是创建当前EventLoop对象的线程，直接执行；
* 否则，用户任务函数入队列pendingFunctors_成为一个pending functor，在loop循环中排队执行
*/
void EventLoop::runInLoop(Functor cb){
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

/**
* 排队进入pendingFunctors_，等待执行
* @param cb 用户任务函数
* @note 如果当前线程不是创建当前EventLoop对象的线程，或者正在调用pending functor，
* 就唤醒loop线程，避免loop线程阻塞.
*/
void EventLoop::queueInLoop(Functor cb){
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pendingFuntors_.push_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop线程
    /** 
     * TODO:
     * std::atomic_bool callingPendingFunctors_; 标志当前loop是否有需要执行的回调操作
     * 这个 || callingPendingFunctors_ 比较有必要，因为在执行回调的过程可能会加入新的回调
     * 则这个时候也需要唤醒，否则就会发生有事件到来但是仍被阻塞住的情况
     */
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t one=1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n!=sizeof one) {
        //log
        std::cout << "EventLoop::wakeup writes " << n << " bytes instead of 8" << std::endl;
    }
}

size_t EventLoop::queueSize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return pendingFuntors_.size();
}

//有2处可能导致loop线程阻塞：
//1）Poller::poll()中调用poll(2)/epoll_wait(7) 监听fd，没有事件就绪时；
//2）用户任务函数调用了可能导致阻塞的函数；
//而当EventLoop加入用户任务时，loop循环是没办法直接知道的，要避免无谓的等待，
//就需要及时唤醒loop线程，使用eventfd技术，来唤醒线程。
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n!=sizeof one) {
        //log
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    /**
     * TODO:
     * 如果没有生成这个局部的 functors
     * 则在互斥锁加持下，我们直接遍历pendingFunctors
     * 其他线程这个时候无法访问，无法向里面注册回调函数，增加服务器时延
     */
    {
        std::lock_guard<std::mutex> lk(mutex_);
        functors.swap(pendingFuntors_);
    }
    for (auto &func : functors) {
        func();
    }
    callingPendingFunctors_ = false;
}

// void EventLoop::printActiveChannels()const{
//     for(const auto c:activeChannels_){
//         //log
//         std::cout<<"{"<<c->reventsToString()<<"}";
//     }
// }
