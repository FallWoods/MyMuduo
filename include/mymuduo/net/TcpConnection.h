#pragma once

#include "noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "InetAddress.h"
#include "Socket.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;

// TcpConnection类是muduo最核心的类，唯一默认用shared_ptr来管理的类，唯一继承自enable_shared_from_this的类。
// 这是因为其生命周期模糊：可能在连接断开时，还有其他地方持有它的引用，
// 贸然delete会造成空悬指针。只有确保其他地方没有持有该对象的引用的时候，才能安全地销毁对象
/**
* Tcp连接, 为服务器和客户端使用.
* 接口类, 因此不要暴露太多细节.
* 
* @note 继承自std::enable_shared_from_this的类, 可以用getSelf返回用std::shared_ptr管理的this指针
*/
class TcpConnection : public noncopyable,
                    public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop,
                    const std::string& name,
                    int sockfd,
                    const InetAddress& localAddr,
                    const InetAddress& peerAddr);
    ~TcpConnection();
    //获得其所属的subloop
    EventLoop* getLoop() const {  return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }
    // NOT thread safe, may race with start/stopReadInLoop
    bool isReading() const { return reading_; }
    
    std::string getTcpInfoString() const;

    // 发送消息给连接对端
    void send(const void* message, int len);
    // void send(std::string&& message);
    void send(const std::string& message);
    // void send(Buffer&& message);
    void send(Buffer* message);
    
    //关闭连接
    void shutdown(); // NOT thread safe, no simultaneous calling

    void setTcpNoDElay(bool on) { socket_->setTcpNoDelay(on); }

    //保存用户自定义的回调函数
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(const CloseCallback& cb){
        closeCallback_ = cb;
    }

    // called when TcpServer accepts a new connection
    void connectEstablished();   // should be called only once
    // called when TcpServer has removed me from its map
    void connectDestroyed();  // should be called only once

private:
    enum StateE{
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void setState(StateE s){
        state_ = s;
    }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();  // 处理关闭连接事件
    void handleError();

    // loop线程中排队发送消息
    void sendInLoop(const std::string& message);
    void sendInLoop(const void* message, size_t len);
    
    // loop线程中排队关闭写连接
    void shutdownInLoop();
    
    // 连接所属的loop
    EventLoop* loop_;
    const std::string name_;  //Tcp连接名称
    std::atomic_int state_;
    bool reading_ = true;           // 连接是否正在监听读事件
   
    // we don't expose those classes to client.
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddress localAddr_;   // 本服务器地址
    const InetAddress peerAddr_;    // 对端地址

    /**
     * 用户自定义的这些事件的处理函数，然后传递给 TcpServer 
     * TcpServer在创建 TcpConnection对象时候设置这些回调函数到 TcpConnection中
     */
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;  // 高水位回调
    CloseCallback closeCallback_;  // 关闭连接回调
    
    size_t highWaterMark_;  // 高水位阈值
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};