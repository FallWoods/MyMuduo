#pragma once

#include "noncopyable.h"
#include "Channel.h"
#include "Timestamp.h"
// #include "EventLoop.h"

#include <vector>
#include <map>

// 支持poll, epoll
// 只有拥有EventLoop的IO线程，才能调用EventLoop所拥有的Poller对象的接口，
// 因此考虑Poller的线程安全不是必要的。
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;
    explicit Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // 需要交给派生类实现的接口
    // polls the I/O events
    // Must be called in the loop thread
    /*
     * 监听函数，根据激活的通道列表，监听指定fd的相应事件
     * 对于PollPoller会调用poll, 对于EPollPoller会调用epoll_wait
     *
     * 返回调用完epoll_wait/poll的当前时刻（Timestamp对象）
     */
    virtual Timestamp poll(int timeoutsMs, ChannelList& activeChannels) = 0;

    // changes the interested I/O events
    // Must be called in the loop thread
    virtual void updateChannel(Channel* channel) = 0;
    // 删除监听的通道
    virtual void removeChannel(Channel* channel) = 0;
    // 判断 channel是否注册到 poller当中
    virtual bool hasChannel(Channel* channel);
    /*
     * 断言所属EventLoop为当前线程.
     * 如果断言失败，将终止程序（LOG_FATAL）
     */
    // void assertInLoopThread(){
    //     ownerLoop_->assertInLoopThread();
    // }
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    /*  Channel:
     *  该类型保存fd和需要监听的events，以及各种事件回调函数（可读/可写/错误/关闭等）
     */
    using ChannelMap = std::map<int, Channel*>;
    // 保存所有事件的Channel，一个fd绑定一个Channel
    ChannelMap channels_;

private:
    // void fillActiveChannels(int numEvents,ChannelList& activeChannels)const;
    // using PollFdList=std::vector<struct pollfd>;
    EventLoop* ownerLoop_;  // 它所属的EventLoop
    // PollFdList pollfds_; // 记录要监视的文件描述符
};