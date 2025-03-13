#pragma once

#include "noncopyable.h"

#include <functional>
#include <poll.h>
#include <memory>


class EventLoop;
class Timestamp;

//Poller监听的对象就是Channel，一个Channel对象绑定了一个fd（文件描述符），
//用来监听发生在fd上的事件，事件包括空事件（不监听）、可读事件、写完成事件。
//当fd上发生监听事件时，对应Channel对象就会被Poller放入激活队列（activeChannels_），进而在loop循环中调用封装在Channel的相应回调来处理事件。
//每个Channel对象从始至终只负责一个文件描述符（fd）的IO事件分发，
//但不拥有fd，也不会在析构时关闭fd。而是由诸如TcpConnection、Acceptor、EventLoop等，
//这些需要监听指定文件描述符上事件的类，将fd通过构造函数传递给Channel。
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的回调函数
    void handleEvent(Timestamp receiveTime);
    
    //设置事件回调，由Channel对象持有者处理Channel事件时回调
    void setReadCallback(const ReadEventCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }

    // Tie this channel to the owner object managed by shared_ptr,
    // prevent the owner object being destroyed in handleEvent.
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    //user by poller
    void set_revents(int revt) { revents_=revt; }

    //for poller
    int index() { return index_; }
    //for poller
    void set_index(int idx) { index_=idx; }

    void enableReading() {
        events_ |= kReadEvent;
        update();
    }
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll() {
        events_ &= kNoneEvent;
        update();
    }

    //是否监听了任何一个事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    //是否正在监听可写事件
    bool isWriting() const { return events_ & kWriteEvent; }
    //是否正在监听可读事件
    bool isReading() const { return events_ & kReadEvent; }

    EventLoop* ownerloop() { return loop_; }
    /* 从EventLoop中移除当前通道.
     * 建议在移除前禁用所有事件
     */
    void remove();

private:
    /* update()将调用EventLoop::updateChannel更新监听的通道 */
    void update();
    // 根据不同的事件源激活不同的回调函数，来处理事件
    void handleEventWithGuard(Timestamp receiveTime);
    
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;  //当前Channel对象属于哪个EventLoop对象
    const int fd_;
    int events_;       //poll关心的事件
    int revents_;      //实际发生的事件，Poller类返回
    
    //used by Poller,
    //在PollPoller中，记录Channel所管理的文件描述符在pollfds_数组中的下标
    //在EpollPoller中，用来标记当前Channel对象的状态，详见EpollPoller.cc
    int index_; 

    // 弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)，防止循环引用
    std::weak_ptr<void> tie_;
    bool tied_;  // 标志此 Channel 是否被调用过 Channel::tie 方法

    ReadEventCallback readCallback_;  /* 可读事件回调 */
    EventCallback writeCallback_;     /* 可写事件回调 */
    EventCallback errorCallback_;     /* 错误事件回调 */
    EventCallback closeCallback_;     /* 关闭事件回调 */
};