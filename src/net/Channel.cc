#include "Channel.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include <iostream>

#include <sstream>
#include <sys/epoll.h>
//在类外定义类的静态成员
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
	  fd_(fd),
	  events_(POLLIN),
	  revents_(0),
	  index_(-1), //初始化时，状态为kNew，表示还没被添加到Poller的map中
	  tied_(false){}

Channel::~Channel(){}

// 在TcpConnection建立得时候会调用，用tied_绑定指向TcpConnection的shared_ptr
void Channel::tie(const std::shared_ptr<void>& obj) {
	tie_ = obj;
  	tied_ = true;
}

/**
* 处理激活的Channel事件
* @details Poller中监听到激活事件的Channel后, 将其加入激活Channel列表,
* EventLoop::loop根据激活的Channel回调对应事件处理函数.
* @param recevieTime Poller中调用epoll_wait/poll返回后的时间. 用户可能需要该参数.
*/
void Channel::handleEvent(Timestamp receiveTime) {
    std::cout << "new Event" << std::endl;
    /**
     * 调用了Channel::tie会设置tid_=true
     * 而TcpConnection::connectEstablished会调用channel_->tie(shared_from_this());
     * 所以对于TcpConnection::channel_需要多一份强引用的保证以免用户误删TcpConnection对象
     */
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
        // guard为空情况，说明Channel的TcpConnection对象已经不存在了
    } else {
        handleEventWithGuard(receiveTime);
    }
}

//根据发生的事件执行相应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    //log
    // printf("%s\n",reventsToString().data());
    // 对端关闭事件
    // 当对端通过shutdown关闭写端或close关闭连接，epoll同时触发POLLHUP和POLLIN
    //为什么不直接调用closeCallback?因为对端可能先发送数据过来，然后直接断开连接，此时，输入缓冲区中有数据，我们不能直接关闭连接，要先读
    //但是，就算对方只是断开连接，也会同时触发POLLHUP和POLLIN，因此，我们无法区分断开连接时，有没有输入数据，所以，直接不监听POLLHUP事件了
    //只监听读事件，在handleRead()内部，做一个判断，如果read的返回值为0，说明读到了EOF，对方断开了连接，此时，调用handleClose().
    // if (revents_ & POLLHUP) {
    //     //调用关闭回调
    //     if (readCallback_) {
    //         readCallback_(receiveTime);
    //     }
    //     if (closeCallback_) {
    //         closeCallback_();
    //     }
    //     std::cout<< "Connection has been closed" << std::endl;
    // }
    if (revents_ & POLLNVAL) {
        //无效请求，fd没开
        //log
        printf("Channel::handle_event() POLLNVAL");
    } else if (revents_ & POLLERR) {
        // 错误条件, 或 无效请求, fd没打开
        if (errorCallback_) {
            errorCallback_();
        }
    } else if (revents_ & (POLLIN | POLLPRI)) {
        // 读事件
        // 有待读数据, 或 紧急数据(e.g. TCP带外数据)或 对方断开了连接（发送一个EOF）
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    } else if (revents_ & POLLOUT) {
        // 可写事件
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

void Channel::update() {
    // 通过该channel所属的EventLoop，调用poller对应的方法，注册fd的events事件
    loop_->updateChannel(this);
}

void Channel::remove(){
    // 在channel所属的EventLoop中，把当前的channel删除掉
  	loop_->removeChannel(this);
}

// std::string Channel::eventsToString()const{
// 	return eventsToString(fd_,events_);
// }

// std::string Channel::reventsToString()const{
// 	return eventsToString(fd_,revents_);
// }

// std::string Channel::eventsToString(int fd, int ev){
//   	std::ostringstream oss;
//   	oss << fd << ": ";
//  	if (ev & POLLIN) oss << "IN ";
//   	if (ev & POLLPRI) oss << "PRI ";
//   	if (ev & POLLOUT) oss << "OUT ";
//   	if (ev & POLLHUP) oss << "HUP ";
//   	if (ev & POLLRDHUP) oss << "RDHUP ";
//     if (ev & POLLERR) oss << "ERR ";
// 	if (ev & POLLNVAL) oss << "NVAL ";
//   	return oss.str();
// }
