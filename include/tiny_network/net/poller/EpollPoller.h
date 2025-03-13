#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

class Channel;

class EpollPoller : public Poller {
    using EventList = std::vector<struct epoll_event>;
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller()override;
    Timestamp poll(int timeoutMs, ChannelList& activeChannels) override;
    // ADD/MOD/DEL
    void updateChannel(Channel* channel) override;
    //DEL
    void removeChannel(Channel* channel) override;

private:
    // events_数组初始大小，默认监听事件数量
    static const int kInitEventListSize = 16;
    // 将操作(EPOLL_CTL_Add/MOD/DEL)转换成字符串
    static const char* operationToString(int op);
    //poll返回后将就绪的fd对应的Channel添加到activeChannels中
    void fillActiveChannels(int numEvents, ChannelList& activeChannels) const;
    //由updateChannel/removeChannel调用，真正执行epoll_ctl()控制epoll的函数
    void update(int operation,Channel* channel);
    
    // epoll例程的文件描述符
    int epollfd_;
    // 用来保存epoll_wait()的结果
    EventList events_;
};