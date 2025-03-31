#include "EpollPoller.h"
#include "Channel.h"
#include "CurrentThread.h"

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <iostream>

const int EpollPoller::kInitEventListSize;

namespace{
// index的3中可能的值

// 表明此Channel对象既没有在Poller的channels_中，也没有被注册在epoll例程中，对这个Channel调用updateChannel(),只能是既要把他加入到channels_中，也要把它注册到epollfd中
const int kNew = -1;

/**
 * 表明此Channel对象以已被注册到epollfd，且在channels_中，如果对这种Channel进行updateChannel()操作，
 * 两种可能：
 * ①把他从epoll例程中删除，暂时不再监听它，令他的index为kDeleted，以后还可能再监听他
 * ②更新epoll例程中此文件描述符监听的事件
 */
const int kAdded = 1;

// 表明此Channel对象已从epoll例程中删除，当前暂时不再监听它，但还包留在channels_中，未被完全删除，只有removeChannel()才是完全删除
// 对这种Channel调用updateChannel()操作，只能是把它重新注册回epollfd
const int kDeleted = 2;
}

/**
 * 创建epoll例程，并没有用epoll_create，而是用 epoll_create1。
 * 原因在于： epoll_create1在打开epoll文件描述符时，可以直接指定FD_CLOEXEC选项，
 * 相当于open时指定O_CLOSEXEC。另外，epoll_create的size参数在Linux2.6.8以后，
 * 就已经没用了（>0即可），内核会实现自动增长内部数据结构以描述监听事件。
 */
EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize){
    if (epollfd_ < 0) {
        //log
        std::cout << "EPollPoller::EPollPoller" << std::endl;
        std::cout << "epoll_create() error:" << errno <<std::endl;
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList& activeChannels) {
    // 高并发情况经常被调用，影响效率，使用debug模式可以手动关闭
    std::cout << "fd total count " << channels_.size() <<std::endl;
    int numEvents = epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        //log
        std::cout << numEvents << " events happend" <<std::endl;
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            //如果当前events已被填满，则扩充空间
            events_.resize(events_.size() * 2);
        }
    } else if(numEvents == 0) {
        //log
        std::cout << "nothing happended,timeout" <<std::endl;
    } else {
        if(savedErrno != EINTR){
            errno = savedErrno;
            std::cout << "EpollPoller::poll() failed" << std::endl;
        }
    }
    return now;
}

// 填写活跃的Channel
void EpollPoller::fillActiveChannels(int numEvents, ChannelList& activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = channels_.find(events_[i].data.fd)->second;
        channel->set_revents(events_[i].events);
        activeChannels.push_back(channel);
    }
}

void EpollPoller::updateChannel(Channel* channel) {
    const int index = channel->index();
    // log
    if (index == kNew || index == kDeleted) {
        int fd = channel->fd();
        if (index == kNew) {
            channels_[fd] = channel;
        }else{
            //index==kDeleted，只是从epoll例程中被删除了，还在channels中
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        // 修改channel的状态，此时是已添加状态
        channel->set_index(kAdded);
        // 向epoll对象加入channel
        update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->isNoneEvent()) {
            //如一个通道不关心任何事件，要对它进行更新，只能是暂时不再监听它
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }else{
            // 如果它还关心某些事件，则应更新监听事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 执行添加、删除（指的是把它从epoll例程中给删除）、修改操作，只能被updateChannel()调用
void EpollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    ::memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;
    event.data.fd = fd;
    //log
    if (operation == EPOLL_CTL_DEL) {
        if (::epoll_ctl(epollfd_, operation, fd, NULL) < 0) {
            std::cout << "epol_ctl op= "<< operationToString(operation) << std::endl;
        }
    } else {
        if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
            std::cout << "epoll_ctl op= "<< operationToString(operation) << std::endl;
        }
        std::cout << "successfully " << operationToString(operation) << " in pid= "<< CurrentThread::tid() <<std::endl;
    }
}

//永久删除某个Channel对象，包括从epoll例程和channels_中都删除
void EpollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    channels_.erase(fd);
    if (index == kAdded) {
         // 如果此fd已经被添加到epoll例程中，则还需从epoll例程中删除
        update(EPOLL_CTL_DEL, channel);
    }
    // 重新设置channel的状态为未被Poller注册
    channel->set_index(kNew);
}

const char* EpollPoller::operationToString(int op) {
    switch (op){
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false && "ERROR op");
        return "Unknown Operation";
    }
}