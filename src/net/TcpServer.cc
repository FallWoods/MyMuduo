#include "TcpServer.h"
#include "TcpConnection.h"
#include "SocketsOps.h"

#include <sstream>
#include <iostream>

TcpServer::TcpServer(EventLoop* loop,const InetAddress& listenAddr,
                        const std::string& nameArg,Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      writeCompleteCallback_(),
      threadInitCallback_(),
      started_(0),
      nextConnTd_(1){
    // 设置用于新建连接的回调，当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生执行handleRead()调用TcpServer::newConnection回调建立新连接
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnnection, this, _1, _2));
}

TcpServer::~TcpServer() {
    //log
    std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing"<<std::endl;
    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        // 把原始的智能指针复位,让栈空间的TcpConnectionPtr conn指向该对象，当conn出了其作用域,即可释放智能指针指向的对象
        item.second.reset();
        // 销毁连接
        conn->getLoop()->runInLoop(
                std::bind(&TcpConnection::connectDestroyed, conn));
    }
}


// 启动TcpServer, 初始化线程池, 连接接受器Accept开始监听(Tcp连接请求)
void TcpServer::start() {
    if (started_.exchange(1) == 0) {
        // 启动底层的lopp线程池
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listening());
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

/**
 * 此函数被设置为Acceptor的newConnection,有一个新用户连接，
 * acceptor会执行这个回调操作，新建一个TcpConnection对象, 用于连接管理，并将此连接关联的Channel分发给subLoop去处理
 * @details 新建的TcpConnection对象会加入内部ConnectionMap.
 * @param sockfd accept返回的连接fd (accepted socket fd)
 * @param peerAddr 对端ip地址信息
 * @note 必须在所属loop线程运行
 */
void TcpServer::newConnnection(int sockfd, const InetAddress& peerAddr) {
    // 轮询算法，从EventLoop线程池中，取出一个subLoop来管理connfd对应的channel,便于均衡各EventLoop负责的连接数
    EventLoop* ioLoop = threadPool_->getNextLoop();
    // 设置连接对象名称, 包含基础名称+ip地址+端口号+连接Id，因为要作为ConnectionMap的key, 要确保运行时唯一性
    std::ostringstream sbuf;
    sbuf << "-" << ipPort_.c_str() << "#" << nextConnTd_;
    ++nextConnTd_;
    std::string connName = name_ + sbuf.str();
    //log
    std::cout << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from " << peerAddr.toIpPort() <<std::endl;
    //获取套接字在本地的地址信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0) {
        std::cout << "sockets::getLocalAddr() failed" <<std::endl;
    }
    InetAddress localAddr(local);

    TcpConnectionPtr conn(std::make_shared<TcpConnection>(ioLoop, connName, sockfd,
                            localAddr, peerAddr));
    connections_[connName] = conn;

    // 下面的3个回调都是用户设置给TcpServer => TcpConnection的，至于Channel绑定的则是TcpConnection设置的四个，handleRead,handleWrite... 这下面的回调用于handlexxx函数中
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置如何关闭连接的回调
    conn->setCloseCallback(
            std::bind(&TcpServer::removeConnnection, this, std::placeholders::_1));
    
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    //log
    std::cout << "TcpServer::removeConnectionInLoop [" << name_.c_str() << "] - connection " << conn->name().c_str() <<std::endl;
    // 从ConnectionMap中擦除待移除TcpConnection对象
    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}