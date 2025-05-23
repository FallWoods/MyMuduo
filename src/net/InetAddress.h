#pragma once

#include "copyable.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

/**
 * sockaddr_in的包装类
 * POD(Plain Old Data)接口类，便于C++和C之间数据类型的兼容
 */
class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};