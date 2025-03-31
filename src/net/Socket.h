#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : public noncopyable {
public:
    explicit Socket(int fd) : sockfd_(fd){}

    //Socket(Socket&& sock);

    ~Socket();

    int fd() const { return sockfd_; }

    // 获取tcp信息, 存放到tcp_info结构
    bool getTcpInfo(struct tcp_info*) const;
    // 获取字符串形式(NUL结尾)的tcp信息, 存放到字符数组buf[len]
    bool getTcpInfoString(char* buf,int len) const;
    // 给socketfd绑定本机ip地址，核心为调用bind()，失败则终止程序
    void bindAddress(const InetAddress& addr);

    void listen();
    //接受连接请求, 返回conn fd(连接文件描述符). 核心调用accept(2).
    int accept(InetAddress* peeraddr);
    //关闭单向写连接
    void shutdownWrite();

    void setTcpNoDelay(bool on);

    void setReuseAddr(bool on);

    void setReusePort(bool on);
 
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};