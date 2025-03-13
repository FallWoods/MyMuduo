#include "Poller.h"
#include "EpollPoller.h"

#include <stdlib.h>

//获取默认的Poller

Poller* Poller::newDefaultPoller(EventLoop *loop){
    if (::getenv("MUDUO_USE_POLL")) {
        // 生成poll实例
        return nullptr;
    } else {
        // 生成epoll实例
        return new EpollPoller(loop);
    }
}
