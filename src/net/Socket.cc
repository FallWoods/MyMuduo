#include "Socket.h"
#include "SocketsOps.h"
#include "InetAddress.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

Socket::~Socket(){
    sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info* tcpi) const {
    socklen_t len = sizeof(*tcpi);
    memset(tcpi, 0, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

//将从getTcpInfo得到的TcpInfo转换为字符串, 存放到长度为len的buf中.
bool Socket::getTcpInfoString(char* buf,int len) const {
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok) {
        snprintf(buf,static_cast<size_t>(len),"unrecovered=%u "
        "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
        "lost=%u retrans=%u rtt=%u rttvar=%u "
        "sshthresh=%u cwnd=%u total_retrans=%u",
        tcpi.tcpi_retransmits, // 重传数, 当前待重传的包数, 重传完成后清零
        tcpi.tcpi_rto,         // 重传超时时间(usec) Retransmit timeout in usec
        tcpi.tcpi_ato,         // 延时确认的估值(usec) Predicted tick of soft clock in usec
        tcpi.tcpi_snd_mss,     // 发送端MSS(最大报文段长度)
        tcpi.tcpi_rcv_mss,     // 接收端MSS(最大报文段长度)
        tcpi.tcpi_lost,        // 丢失且未恢复的数据段数 Lost packets
        tcpi.tcpi_retrans,     // 重传且未确认的数据段数 Retransmitted packets out
        tcpi.tcpi_rtt,         // 平滑的RTT(usec)　Smoothed round trip time in usec
        tcpi.tcpi_rttvar,      // 1/4 mdev (usec) Medium deviation
        tcpi.tcpi_snd_ssthresh,// 慢启动阈值
        tcpi.tcpi_snd_cwnd,    // 拥塞窗口
        tcpi.tcpi_total_retrans// 本连接的总重传个数 Total retransmits for entire connection
        );
    }
    return ok;
}

void Socket::bindAddress(const InetAddress& addr) {
    bind(sockfd_, (sockaddr *)addr.getSockAddr(), sizeof(sockaddr_in));
}
void Socket::listen() { ::listen(sockfd_, 256); }

int Socket::accept(InetAddress* peeraddr) {
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    // fixed : int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    // int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    else
    {
        std::cout << "accept4() failed" <<std::endl;
    }
    return connfd;
}

void Socket::shutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setRuesAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval)); 
    if (ret < 0 && on) {
        //log
    }
#else
    if (on) {
        //log
    }
#endif
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}