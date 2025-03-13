#include "EventLoop.h"
#include "TcpServer.h"

#include <iostream>
// #include "Logging.h"
// #include "AsyncLogging.h"

class EchoServer {
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的subloop线程数量
        // server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn)   
    {
        if (conn->connected())
        {
            std::cout << "Connection UP : " << conn->peerAddress().toIpPort().c_str() << std::endl;
        }
        else
        {
            std::cout << "Connection DOWN : " << conn->peerAddress().toIpPort().c_str() << std::endl;
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        std::cout << conn->name() << " echo " << msg.size() << " bytes, "
                 << "data received at " << time.toFormattedString() << std::endl;
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main() 
{

    std::cout << "pid = " << getpid() <<std::endl;
    EventLoop loop;
    InetAddress addr(9090);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();

    return 0;
}