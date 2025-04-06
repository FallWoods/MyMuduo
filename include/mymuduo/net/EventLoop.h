#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "TimerId.h"
#include "TimerQueue.h"

#include <thread>
#include <stdio.h>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

class Channel;
class Poller;

//Reactor模式：one loop per thread，每个线程一个EventLoop,主要包含了两大模块，channel、poller
class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    /* loop循环, 运行一个死循环.
     * 必须在创建当前对象的线程中运行.
     */
    void loop();

    //退出loop循环
    //如果通过原始指针调用，不是100%线程安全的
    //最好通过std::shared_ptr<EventLoop>调用
    void quit();

    //判断当前线程是否为此对象所属的线程，如果不是，终止线程
    // void assertInLoopThread();

    //判断当前线程是否为此对象所属的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    // 轮询返回时 的时刻
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    // 返回迭代的次数
    int64_t iterator() const { return iteration_; }
    //正在排队的回调cb的个数
    size_t queueSize()const;

    //确保cb是在loop线程内运行
    //如果在loop线程中, 立即运行回调cb.
    //如果没在loop线程, 就会唤醒loop, (排队)运行回调cb.
    //因此, 在其他线程中调用该函数是安全的
    void runInLoop(Functor cb);

    /**
     * 把cb放入队列，唤醒loop所在的线程执行cb
     * 实际情况：
     * 在mainLoop中获取subLoop指针，然后调用相应函数
     * 在queueLoop中发现当前的线程不是创建这个subLoop的线程，将此函数装入subLoop的pendingFunctors容器中
     * 之后mainLoop线程会调用subLoop::wakeup向subLoop的eventFd写数据，以此唤醒subLoop来执行pengdingFunctors
     */
    void queueInLoop(Functor cb);

    //internal usage
    //唤醒loop线程, 没有事件就绪时, loop线程可能阻塞在poll()/epoll_wait()
    void wakeup();

    //定时任务相关函数

    //调用此函数的不必是loop线程，但保证具体将Timer对象添加进TImerQueue的一定是loop线程
    /**
    * 定时功能，由用户指定绝对时间
    * @details 每为定时器队列timerQueue添加一个Timer,
    * timerQueue内部就会新建一个Timer对象, TimerId就保含了这个对象的唯一标识(序列号)
    * @param time 时间戳对象, 单位us
    * @param cb 超时回调函数. 当前时间超过time代表时间时, EventLoop就会调用cb
    */
    void runAt(Timestamp when, const Functor& cb) {
        timerQueue_->addTimer(cb, when, 0.0);
    }

    //在当前时间点延迟delay后运行回调cb，从其他线程调用是安全的
    /**
    * @param delay 相对时间, 单位s, 精度1us(小数)
    * @param cb 超时回调
    */
    void runAfter(double delay, const Functor& cb) {
        timerQueue_->addTimer(cb, Timestamp::now() + delay, 0.0);
    }

    //每隔interval 秒周期调用回调cb
    /**
    * 定时功能, 由用户指定周期, 重复运行
    * @param interval 运行周期, 单位s, 精度1us(小数)
    * @param cb 超时回调
    */
    //从其他线程调用是安全的
    void runEvery(double interval, const Functor& cb) {
        timerQueue_->addTimer(cb, Timestamp::now() + interval, interval);
    }
    
    // 取消指定的定时器，TimerId唯一标识定时器Timer
    // 从其他线程调用是安全的
    // void cancel(TimerId timerfd);

    //更新Poller监听的channel,只能在channel所属的loop线程中调用
    void updateChannel(Channel* channel);
    //移除Poller监听的channel,只能在channel所属的loop线程中调用
    void removeChannel(Channel* channel);
    //判断poller是否正在监听channel，只能在channel所属的loop线程中调用
    bool hasChannel(Channel* channel);

    //用于获取当前线程的EventLoop对象指针
    static EventLoop* getEventLoopOfCurrentThread();
private:
    // //终止程序，当前线程不是创建当前EventLoop对象的线程时,由assertInLoopThread()调用
    // void abortNotInLoopThread();

    //wake up
    // 被设置为唤醒EventLoop后的回调， 其实什么都不做，只是为了让当前线程被唤醒，然后继续沿着loop运行，
    // 发现没有什么事做，就执行pendingFuntors_中的回调。
    void handleRead();
    //处理pending函数
    void doPendingFunctors();
    //打印激活通道的事件信息，用于debug
    // void printActiveChannels() const;

    using ChannelList = std::vector<Channel*>;

    const pid_t threadId_; //当前对象所属线程id
    //atomic, true表示在loop循环执行中
    std::atomic_bool looping_;
    //退出loop循环的条件 
    std::atomic_bool quit_;
    //atomic，表示loop循环正在调用pedning函数
    std::atomic_bool callingPendingFunctors_;
    Timestamp pollReturnTime_; //poll()返回时刻
    //轮询器，每个EventLoop一个
    std::unique_ptr<Poller> poller_;
    //定时器队列
    std::unique_ptr<TimerQueue> timerQueue_;

    /**
     * TODO:eventfd用于线程通知机制，libevent和我的webserver是使用sockepair
     * 作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 
     * 通过该成员唤醒subLoop处理Channel
     */
    // 唤醒loop线程的eventfd，对于每一个loop，他可能在poll的过程中阻塞（没有就绪的文件描述符），
    //为了唤醒loop线程，在创建每一个loop的时候，给其创建一个wakeFd，并将对应的channel添加到poll中监听读事件，当我们需要唤醒此loop线程时，就通过wakeup()向此文件描述符写入一些东西，此时，wakeupFd触发读事件，IO复用函数返回，loop线程接触阻塞
    int wakeupFd_;
    // 用来管理wakeupFd_的Channel，用于唤醒loop线程
    std::unique_ptr<Channel> wakeupChannel_;
    
    ChannelList activeChannels_; // 活跃的Channel列表
    // 当前正在处理的活跃channel
    Channel* currentActiveChannel_;
    
    mutable std::mutex mutex_; // 用于保护pendingFuntors_
    // 待调用函数列表，用于存放不在loop线程中调用的函数，
    // 当这些函数被调用时，发现调用它们的线程不是loop线程，则将这些函数存放于此，由loop线程调用
    std::vector<Functor> pendingFuntors_;
    int64_t iteration_; //loop循环次数
};