#include "SocketsOps.h"

#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
namespace{

typedef struct sockaddr SA;

#if VALGRIND || defined (NO_ACCEPT4)
void  setNonBlockAndCloseOnExec(int sockfd){
    //non-block
    int flags=::fcntl(sockfd,F_GETFL,0);
    flags |= O_NONBLOCK;
    int ret=::fcntl(sockfd,F_SETFL,flags);
    //FIXME check

    //close-on-exec
    flags=::fcntl(sockfd,F_GETFL,0);
    flags |= FD_CLOEXEC;
    ret=::fcntl(sockfd,F_SETFL,flags);
    //FIXME check

    void(ret);
}
#endif

}


namespace sockets{


/**
 * 提供不同地址类型之间的转型，如sockaddr_in/sockaddr_in6/sockaddr*，要求成员内存布局必须是一样的。
 * 这也是为什么前面用static_assert来断言sockaddr_in/sockaddr_in6成员偏移的原因（offsetof），
 * 因为如果成员偏移不一样，也就是说对象的内存布局不一样，通过指针直接转型是不对的。
 * 单独的static_cast，是无法将一种指针类型转换为另一种指针类型的，需要先利用implicit_cast（隐式转型）/static_cast（显式转型）
 * 将指针类型转换为void/const void （无类型）指针，然后才能转换为模板类型指针。
 * 而reinterpret_cast可以直接做到，但reinterpret_cast通常并不安全，编译期也不会在编译期报错，通常不推荐使用。
 */
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr){
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr){
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr){
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

struct sockaddr* sockaddr_cast(struct sockaddr_in* addr){
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr)
{
  return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

const struct sockaddr_in6* sockaddr_in6_cast(const sockaddr* addr){
    return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}


/**
* 创建一个非阻塞sock fd
* @param family 协议族, 可取值AF_UNIX/AF_INET/AF_INET6 etc.
* @return 成功, 返回sock fd; 失败, 程序终止(LOG_SYSFATAL)
*/
int createNonblockingOrDie(sa_family_t family){
#if VALGRIND  // a kind of memory test tool
    int sockfd=socket(family,SOCK_STREAM,0);
    if(sockfd < 0) {
        //log
    }
    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd=socket(family,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd  < 0) {
        //log
    }
#endif
    return sockfd;
}

int connect(int sockfd,const struct sockaddr* addr){
    return ::connect(sockfd,addr,sizeof(addr));
}

void bindOrDie(int sockfd,const struct sockaddr* addr){
    auto ret= ::bind(sockfd,addr,sizeof(addr));
    if(ret<0){
        //log
    }
}

void listenOrDie(int sockfd){
    int ret=::listen(sockfd,16);
    if(ret<0){
        //log
    }
}

/**
* accept(2)/accept4(2)包裹函数, 接受连接并获取对端ip地址
* @param sockfd 服务器sock fd, 指向本地监听的套接字资源
* @param addr ip地址信息
* @return 由sockfd接收连接请求得到到连接fd
*/
int accept(int sockfd,struct sockaddr_in6* addr){
    socklen_t addrlen=static_cast<socklen_t>(sizeof(*addr));
#if VALGRIND || defined(NO_ACCEPT4) // VALGRIND: memory check tool
    int connfd=::accept(sockfd,(struct sockaddr*)addr,&addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    // set flags for conn fd returned by accept() at one time
    int connfd=::accept4(sockfd,(struct sockaddr*)addr,&addrlen,SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if(connfd<0){
        int savedErrno=errno;
        //log
        switch(savedErrno){
            case EAGAIN:
            case  ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EMFILE:
                // expected errors
                errno=savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                //log
            default:
                //log
                break;
        }
    }
    return connfd;
}

ssize_t read(int sockfd, void* buf, size_t cout){
    return ::read(sockfd, buf, cout);
}

ssize_t readv(int sockfd,const struct iovec* iov,int iovcnt){
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t write(int sockfd,const void* buf,size_t count){
    return ::write(sockfd,buf,count);
}

void close(int sockfd){
    if(::close(sockfd)<0){
        //log
    }
}

void shutdownWrite(int sockfd){
    if(shutdown(sockfd,SHUT_WR)<0){
        //log
    }
}

void toIp(char* buf,size_t size,const struct sockaddr* addr){
    if(addr->sa_family==AF_INET){
        assert(size>=INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4=(struct sockaddr_in*)addr;
        ::inet_ntop(AF_INET,&addr4->sin_addr,buf,size);
    }else{
        assert(size>INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6=(struct sockaddr_in6*)addr;
        ::inet_ntop(AF_INET6,&addr6->sin6_addr,buf,size);

    }
}

void toIpPort(char* buf,size_t size,const struct sockaddr* addr){
    if(addr->sa_family==AF_INET6){
        //ipv6
        buf[0]='[';
        toIp(buf+1,size-1,addr);
        size_t end=::strlen(buf);
        const struct sockaddr_in6* addr6=(struct sockaddr_in6*)addr;
        uint16_t port=ntohs(addr6->sin6_port);
        assert(size>end);
        snprintf(buf+end,size-end,"]:%u",port);
        return;
    }
    //ipv4
    toIp(buf,size,addr);
    size_t end=strlen(buf);
    const struct sockaddr_in* addr4=(struct sockaddr_in*)addr;
    uint16_t port=ntohs(addr4->sin_port);
    assert(size>end);
    snprintf(buf+end,size-end,"%u",port);
}


void fromIpPort(const char* ip,uint16_t port,struct sockaddr_in *addr){
    addr->sin_family=AF_INET;
    addr->sin_port=htons(port);
    if(::inet_pton(AF_INET,ip,&addr->sin_addr)<=0){
        //log
    }
}

void fromIpPort(const char* ip,uint16_t port,struct sockaddr_in6* addr){
    addr->sin6_family=AF_INET6;
    addr->sin6_port=htons(port);
    if(::inet_pton(AF_INET6,ip,&addr->sin6_addr)<=0){
        //log
    }
}


int getSocketError(int sockfd){
    int optval;
    socklen_t optlen=static_cast<socklen_t>(sizeof(optval));
    if(::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)){
        return errno;
    }else{
        return optval;
    }
}

struct sockaddr_in6 getLocalAddr(int sockfd){
    struct sockaddr_in6 localaddr;
    memset(&localaddr,0,sizeof(localaddr));
    socklen_t addrlen=static_cast<socklen_t>(sizeof(localaddr));
    if(::getsockname(sockfd,sockaddr_cast(&localaddr),&addrlen)<0){
        //log
        std::cout << "sockets::getLocalAddr() failed" << std::endl;
    }
    return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd){
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr,0,sizeof(peeraddr));
    socklen_t addrlen=sizeof(peeraddr);
    if(::getpeername(sockfd,sockaddr_cast(&peeraddr),&addrlen)){
        //log
    }
    return peeraddr;
}

/**
 * 检查是否为自连接, 判断连接sockfd两端ip地址信息是否相同，
 * 对于IPv4，地址sin_addr.s_addr是32bit，能用==判断是否相等；
 * 而对于IPv6，地址sin6_addr是28byte，无法用==判断，需要用memcmp来比较二进制位。
 * @param sockfd 连接对应的文件描述符
 * @return true: 是自连接; false: 不是自连接
 */

bool isSelfConnect(int sockfd){
    struct sockaddr_in6 localaddr=getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr=getPeerAddr(sockfd);
    if(localaddr.sin6_family==AF_INET){
        //ipv4
        const struct sockaddr_in* laddr4=reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4=reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_addr.s_addr==raddr4->sin_addr.s_addr &&
        laddr4->sin_port==raddr4->sin_port;
    }else if(localaddr.sin6_family==AF_INET6){
        //ipv6
        return localaddr.sin6_port==peeraddr.sin6_port &&
        memcmp(&localaddr.sin6_addr,&peeraddr.sin6_addr,sizeof(localaddr.sin6_addr))==0;
    }else{
        return false;
    }
}

}
