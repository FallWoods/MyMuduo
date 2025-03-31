#pragma once

#include "noncopyable.h"
#include <cassert>
#include <cstring>
#include <strings.h>
#include <string>

const int kSmallBuffer = 4 * 1024;
const int kLargeBuffer = 4 * 1024 * 1024;

template <int SIZE>
class FixedBuffer : noncopyable {
public:
    FixedBuffer() : cur_(data_) {}
    //在缓冲区的后面增加数据
    void append(const char* buf, size_t len){
        if (static_cast<size_t>(avail()) > len){
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }
    // 缓冲区的起始地址
    const char* data() const { return data_; }
    // 缓冲区数据长度（单位为字节）
    int length() const { return static_cast<int>(cur_ - data_); }

    // 缓冲区的空闲首地址
    char* current() { return cur_; }
    // 返回缓冲区剩余的大小
    int avail() const { return static_cast<int>(end() - cur_); }
    // 往缓冲区添加len字节数据后，调用此函数
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { ::bzero(data_, sizeof data_); }
    // 将缓冲区的内容转换成string
    std::string toString() const { return std::string(data_, cur_ - data_); }
private:
    const char* end() const { return data_ + sizeof(data_); }
    
    char data_[SIZE];
    char* cur_;
};