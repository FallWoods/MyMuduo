#include "Acceptor.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "EventLoop.h"

#include <functional>
#include <iostream>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        std::cout << "listen socket create err " << errno <<std::endl;
        // LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd())
      /*idleFd_(::open("/dev/null",O_RDONLY | O_CLOEXEC))*/{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    /**
     * TcpServer::start() => Acceptor.listen
     * 有新用户的连接，需要执行一个回调函数
     * 因此向封装了acceptSocket_的channel注册回调函数
     */
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this, std::placeholders::_1));
}

Acceptor::~Acceptor() {
    // 从Poller中把感兴趣的事件删除掉
    acceptChannel_.disableAll();
    // 移除用于接收连接的Channel
    // 调用EventLoop->removeChannel => Poller->removeChannel 把Poller的ChannelMap对应的部分删除
    acceptChannel_.remove();
    // ::close(idleFd_);
}

//令用于接收连接的套接字进入监听状态，使能监听Channel读事件
void Acceptor::listen() {
    std::cout << "start listen" << std::endl;
    listening_ = true;
    acceptSocket_.listen();  //让用于接收连接的套接字进入监听状态
    acceptChannel_.enableReading();  //使对应Channel对象能监听读事件
}

// listenfd有事件发生了，就是有新用户连接了
// Acceptor内部有一个Channel成员，当Poller监听到有Tcp连接请求时，就通过Channel的可读事件，
// 在loop线程，来回调Acceptor::handleRead()（Channel对象的readCallback已被设置为Acceptor::handleRead()）。
// 从而将conn fd和IP地址传递给上一层TcpServer，用于创建TcpConnection对象管理Tcp连接。
/**
* 处理读Channel事件, accept连接
* @note 先accept, 然后将相关资源通过回调交由上一层的TcpServer进行处理(管理)
*/
//此函数即为当绑定了用于接收连接的套接字的Channel对象触发读事件时，应调用的回调函数
void Acceptor::handleRead(Timestamp when) {
    std::cout << "new Connection Establised" << when.toFormattedString() << std::endl;
    InetAddress peerAddr;
    //FIXME loop until no more
    // 接受新连接
    std::cout << "new Connection estavlished in Acceptor" << std::endl;
    int connfd = acceptSocket_.accept(&peerAddr);  //接收连接并获取对端地址信息
    if(connfd >= 0) {
        // TcpServer::NewConnectionCallback_
        if(newConnectionCallback_) {
            //如果用于创建TcpConnection的回调函数已经设置好，则调用之
            newConnectionCallback_(connfd, peerAddr);
        } else {
            // LOG_DEBUG("connfd < 0 accept failed");
            std::cout << "no newConnectionCallback() function" <<std::endl;
            sockets::close(connfd);
        }
    }else{
        //发生错误
         /*
         * Read the section named "The special problem of
         * accept()ing when you can't" in libev's doc.
         * By Marc Lehmann, author of libev.
         *
         * The per-process limit of open file descriptors has been reached.
         */
        std::cout << "accept() failed" <<std::endl;
        if(errno == EMFILE){
            //文件描述符资源耗尽错误,使用备用的fd
            // ::close(idleFd_);
            // idleFd_=::accept(acceptSocket_.fd(),NULL,NULL);
            // ::close(idleFd_);

            // reopen /dev/null, it dose not matter whether it succeeds or fails
            // idleFd_=::open("/dev/null",O_RDONLY | O_CLOEXEC);
        }
    }
}