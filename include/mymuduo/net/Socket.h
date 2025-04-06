#pragma once

#include "noncopyable.h"
//采用RTII方式管理sock fd，但本身并不创建sock fd，也不打开它，只负责关闭。
//监听socket, 常用于server socket.

// Socket类不创建sockfd，其含义取决于构造Socket对象的调用者。如果是由调用socket(2)创建的sockfd，
//那就是本地套接字；如果是由accept(2)返回的sockfd，那就是accepted socket，代表一个连接。

// 例如，Acceptor持有的Socket对象，是由socket(2)创建的，代表一个套接字；
// TcpConnection只有一个Socket对象，是由TcpServer在新建TcpConnection对象时传入，而由Acceptor::handleRead()中通过Socket::accept()创建的sockfd参数的实参。

class InetAddress;

class Socket : public noncopyable {
public:
    explicit Socket(int fd) : sockfd_(fd){}

    //Socket(Socket&& sock);

    ~Socket();
    int fd() const { return sockfd_; }
    // 获取tcp信息, 存放到tcp_info结构
    // return true if success.
    bool getTcpInfo(struct tcp_info*) const;
    //获取字符串形式(NUL结尾)的tcp信息, 存放到字符数组buf[len]
    bool getTcpInfoString(char* buf,int len) const;
    //给socketfd绑定本机ip地址，核心为调用bind()，失败则终止程序
    void bindAddress(const InetAddress& addr);

    void listen();
    //接受连接请求, 返回conn fd(连接文件描述符). 核心调用accept(2).
    int accept(InetAddress* peeraddr);
    //关闭单向写连接
    void shutdownWrite();

    //Enable/disable TCP_NODELAY
    void setTcpNoDelay(bool on);
    //Enable/disable SO_REUSEADDR
    void setReuseAddr(bool on);
    //Enable/disable SO_REUSEPORT
    void setReusePort(bool on);
    //Enable/disable SO_KEEPALIVE
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};