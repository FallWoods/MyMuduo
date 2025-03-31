#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

// Acceptor是TcpServer的一个内部类，主要职责是用来获得新连接的fd,
// 新建TcpConnection对象（newConn()）的时候直接传递给TcpConnection的构造函数。

class EventLoop;
class InetAddress;

class Acceptor : public noncopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    //设置用于建立连接后的回调，通过此函数创建一个新的TcpConnection对象，此对象关联与客户端连接的一个套接字
    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }
    //进入监听状态
    void listen();
    //判断是否正在监听
    bool listening() const {  return listening_; }
private:
    void handleRead(Timestamp);   //调用newConnectionCallback_
    
    EventLoop* loop_;       //所属的Loop，就是用户定义的BaseLoop
    Socket acceptSocket_;   //关联的是专门用于接收连接的套接字
    Channel acceptChannel_; //绑定上述套接字的通道，专门用于接收连接
    NewConnectionCallback newConnectionCallback_;  //建立新连接的回调，每当与一个客户端建立连接后，就调用此函数创建对应TcpConnection对象
    bool listening_ = false; //监听状态
    // int idleFd_;  //空闲fd，fd资源不足时，可以空出来一个作为新建连接conn fd
};

// 如果fd资源不够用了，导致accept(2)/accept4(2)创建连接失败，比如达到系统上限，怎么办？
// Accetor用了这样一种技术：先申请一个空闲的fd（idleFd_），等到发生由于fd资源不够用时，
//就把这个备用fd暂时用于accept接收连接，然后再马上关闭，以防止不断产生可读事件（连接请求），
//从而回调相同的失败代码。及早建立连接后并关闭连接，让程序不会频繁响应同一个连接请求
