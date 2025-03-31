#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop):ownerLoop_(loop){}

bool Poller::hasChannel(Channel* channel){
    ChannelMap::iterator res = channels_.find(channel->fd());
    return res != channels_.end() && channel == res->second;
}