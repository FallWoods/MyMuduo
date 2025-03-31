#pragma once

#include "noncopyable.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <atomic>
#include <map>
#include <memory>

// Tcp Server, 支持单线程和thread-poll模型，用户只需要设置好callback，然后调用start()即可。
class TcpServer : noncopyable {
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
    
    enum Option {
        kNoReusePort, //不允许重用本地端口
        kReusePort,   //允许重用本地端口
    };
    TcpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string& nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    const std::string ipPort() const { return ipPort_; }
    const std::string name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    void setThreadNum(int numthreads) { threadPool_->setThreadNum(numthreads); }
    void setThreadInitCallback(const ThreadInitCallback& cb) {  threadInitCallback_ = cb; }
    std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_; }

    void start();

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_=cb; }

    void setMessageCallback(const MessageCallback& cb) { messageCallback_=cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_=cb; }

private:
    /**
     * 同样是连接回调，TcpServer::newConnection()和connectionCallback_的区别：
     * 前者是Acceptor发生连接请求事件时，回调，用来新建一个Tcp连接；
     * 后者是在TcpServer内部新建连接即调用TcpServer::newConnection()时，回调connectionCallback_，用于建立新连接
     */
    void newConnnection(int sockfd,const InetAddress& peerAddr);
    
    // 用于移除一个TcpConnection对象，被设置为TcpConnection的CloseCallback
    void removeConnnection(const TcpConnectionPtr& conn);
    // 在TcpConnection对象所属的loop中具体执行移除操作
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
    
    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    //由具体的服务器类的成员函数包装而成
    ConnectionCallback connectionCallback_;  // 有新连接或连接断开时的回调函数
    MessageCallback messageCallback_;        // 有读写消息时的回调函数
    WriteCompleteCallback writeCompleteCallback_;  // 应用层缓冲区的消息发送完以后的回调函数

    ThreadInitCallback threadInitCallback_;
    std::atomic_int32_t started_;
    int nextConnTd_;             //标识每一个连接的id，每新建一个连接，加1
    ConnectionMap connections_;  //保存所有的连接
};