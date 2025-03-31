#include "Socket.h"
#include "TcpConnection.h"
#include "Channel.h"
#include "Timestamp.h"
#include "EventLoop.h"

#include <iostream>
#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

using namespace std::placeholders;
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if (loop == nullptr) {
        std::cout << "mainLoop is null!" <<std::endl;
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                            const std::string& name,
                            int sockfd,
                            const InetAddress& localAddr,
                            const InetAddress& peerAddr)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024){
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    //log
    std::cout << "TcpConnection::ctor[" << name_.c_str() << "] at fd =" << sockfd <<std::endl;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    //log
    std::cout << "TcpConnection::dtor[" << name_ << "] at fd=" << channel_->fd() << " state=" << static_cast<int>(state_) << std::endl;
    assert(state_ == kDisconnected);
}

//发送数据

// 转发给send(const StringPiece&), 最终转交给sendInLoop(const char*, int)
void TcpConnection::send(const void *message, int len){
    send(std::string(static_cast<const char*>(message), len));
}

/**
* 转交给 sendInLoop(const char*, int)
* 发送消息给对端, 允许在其他线程调用
* @param message 要发送的消息. 
*/
void TcpConnection::send(const std::string& message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            // 当前线程是所属loop线程
            sendInLoop(message.c_str(), message.size());
        } else {
            // 当前线程并非所属loop线程
            // 遇到重载函数的绑定，可以使用函数指针来指定确切的函数
            void(TcpConnection::*fp)(const void*, size_t) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, message.c_str(), message.size()));
        }
    }
}

// 转交给sendInLoop(const char*, int)
// FIXME efficiency!!
void TcpConnection::send(Buffer* buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
    } else {
        // sendInLoop有多重重载，需要使用函数指针确定
        void(TcpConnection::*fp)(const std::string&) = &TcpConnection::sendInLoop;
        //为了在所属的loop线程中执行buf的retrieve，只能执行buf->retriAllAsString()，然后把返回string传给转交的sendInLoop
        loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
    }
}

// 转交给sendInLoop(const void*, int)
void TcpConnection::sendInLoop(const std::string& message) {
    sendInLoop(message.data(),message.size());
}

//send函数用于发送数据，要么直接发送到套接字的输出缓冲区，要么发送到OutBuffer中,发送到OutBuffer中后，要注册可写事件，当事件发生发生时，再在handleWrite中写。
/**
* 在所属loop线程中, 发送data[len]
* @param data 要发送的缓冲区首地址
* @param len　要发送的缓冲区大小(bytes)
* @details 发生write错误, 如果发送缓冲区未满,　对端已发FIN/RST分节 表明tcp连接发生致命错误(faultError为true)
*/
void TcpConnection::sendInLoop(const void* data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    // 如果之前调用过connection的shutdown，写连接已经关闭，则不能再进行发送了
    if (state_ == kDisconnected) {
        std::cout << "disconnected, give up writing" <<std::endl;
        return;
    }
    // write一次, 往对端发送数据, 后面再看是否发生错误, 是否需要高水位回调
    // if no thing output queue, try writing directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        // 如果通道没有监听可写事件, 并且outputBuffer没有待发送数据, 就直接通过socket写
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining -= nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            //< 0,error
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                // EWOULDBLOCK: 输出缓冲区已满, 且fd已设为nonblocking，那么把剩下的数据发送到应用层的输出缓冲区
                //log
                std::cout << "TcpConnection::sendInLoop" <<std::endl;
                if (errno == EPIPE || errno == ECONNRESET) {
                    // EPIPE: reading end is closed; ECONNRESET: connection reset by peer
                    faultError = true; 
                }
            }
        } 
    }
    // 处理剩余数据,剩余数据需要保存到输出缓冲区中，且需要改channel注册写事件
    if (!faultError && remaining > 0) {
        // 没有故障, 并且还有待发送数据, 可能是发送太快, 对方来不及接收
        size_t oldLen = outputBuffer_.readableBytes(); //Buffer中待发送的数据

        if (oldLen + remaining >= highWaterMark_  &&  oldLen < highWaterMark_  && highWaterMarkCallback_) {
            // Buffer及当前要发送的数据量之和超过高水位(highWaterMark)
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // 追加数据到输出缓冲区
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        // 使当前套接字的channel监听可写事件
        if (!channel_->isWriting()) {
            // 如果没有在监听通道可写事件, 就使监听通道可写事件
            // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout
            channel_->enableWriting();
        }
    }
}

//关闭单向写连接，转交函数
void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}
//执行实际工作
void TcpConnection::shutdownInLoop() {
    //有待写数据的时候，会注册可写事件；因此，若没有注册可写事件，说明我们当前没有待写数据，可以关闭写连接
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

//在TcpConnection对象建立以后（即newConnectionCallback之后），调用此函数
void TcpConnection::connectEstablished() {
    setState(kConnected);
    /**
     * TODO:tie
     * channel_->tie(shared_from_this());
     * tie相当于在底层有一个强引用指针记录着，防止析构
     * 为了防止TcpConnection这个资源被误删掉，而这个时候还有许多事件要处理
     * channel->tie 会进行一次判断，是否将弱引用指针变成强引用，变成得话就防止了计数为0而被析构得可能
     */
    channel_->tie(shared_from_this());
    // 向poller注册channel的EPOLLIN读事件
    channel_->enableReading();
    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

// 在TcpServer::newConnection()为新连接创建TcpConnection对象时，
//会将此TcpConnection对象的closeCallback_设置为TcpServer::removeConnection()，而removeConnection()最终会调用TcpConnection::connectDestroyed()来销毁连接资源。
/**
* 销毁当前tcp连接, 移除通道事件
* @note 只有处于已连接状态(kConnected)的tcp连接, 才需要先更新状态, 关闭通道事件监听
*/
void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {  // 只有kConnected的连接, 才有必要采取断开连接动作
        setState(kDisconnected);
        channel_->disableAll();  // 关闭通道事件监听
        connectionCallback_(shared_from_this());  // 调用连接回调
    }
    channel_->remove();
}

/**
* 从socket的输入缓存中读取数据, 交给回调messageCallback_处理
* @param receiveTime 接收到读事件的时间点
* @details 通常是TcpServer/TcpClient运行回调messageCallback_, 将处理机会传递给用户
*/
void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd(), savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if(n == 0) {
        //只有读到了EOF才会返回0，读到了EOF说明对方关闭连接
        handleClose();
    } else {
        errno = savedErrno;
        //log
        std::cout << "TcpConnection::handleRead() failed" <<std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int saveErrno = 0;
        ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                // 说明buffer可读数据都被TcpConnection读取完毕并写入给了客户端,没东西可写，暂时停止监听可写事件
                channel_->disableWriting();
                // 调用用户自定义的写完数据处理函数
                if ( writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                //如果连接已经处于正在关闭状态，则可以关闭单向写连接
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            //写失败
            //log
            std::cout << "TcpConnection::handleWrite() failed" <<std::endl;
        }
    } else {
        // state_不为写状态
        //log
        std::cout << "TcpConnection fd=" << channel_->fd() << " is down, no more writing" <<std::endl;
    }
}

// 被动关闭连接
// 当收到对端FIN=1的消息时即EOF时，触发读事件，本地read返回0,，Tcp连接被动关闭，会触发调用本地TcpConnection::handleClose()
/**
* 处理Tcp连接关闭
* @details 更新状态为kDisconnected, 清除所有事件通道监听
* @note 必须在所属loop线程中运行.
*/
void TcpConnection::handleClose(){
    //log
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);  // 更新Tcp连接状态，设置为关闭连接状态
    channel_->disableAll();   // 停止监听所有通道事件

    TcpConnectionPtr guardTHis(shared_from_this());
    connectionCallback_(guardTHis);  // 连接回调
    closeCallback_(guardTHis);       //　关闭连接回调
}

// Channel在处理通道事件handleEvent时，如果发生错误（既不是读取数据，也不是写数据完成），
// 调用链路：
// Channel::handleEvent() => 检测到错误，调用Channel::errorCallback_ => TcpConnection::handleError()
/**
* 处理tcp连接错误, 打印错误log
* @details 从tcp协议栈获取错误信息
*/
void TcpConnection::handleError(){
    int err = sockets::getSocketError(channel_->fd());
    //log
    std::cout << "cpConnection::handleError name:" << name_ << " - SO_ERROR:" << err <<std::endl;
}

//获取string形式的Tcp连接信息
std::string TcpConnection::getTcpInfoString() const{
    char  buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}