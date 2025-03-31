#pragma once

#include "noncopyable.h"
#include "Channel.h"
#include "Timestamp.h"
// #include "EventLoop.h"

#include <vector>
#include <map>

class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;
    explicit Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutsMs, ChannelList& activeChannels) = 0;

    virtual void updateChannel(Channel* channel) = 0;
    // 删除监听的通道
    virtual void removeChannel(Channel* channel) = 0;
    // 判断 channel是否注册到 poller当中
    virtual bool hasChannel(Channel* channel);

    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    
    // Channel: 该类型保存fd和需要监听的events，以及各种事件回调函数（可读/可写/错误/关闭等）
    using ChannelMap = std::map<int, Channel*>;
    // 保存所有事件的Channel，一个fd绑定一个Channel
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;  // 它所属的EventLoop
};