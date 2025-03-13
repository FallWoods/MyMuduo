#pragma once

#include "FixedBuffer.h"
#include "noncopyable.h"

#include <string>

/**
 *  比如SourceFile类和时间类就会用到
 *  const char* data_;
 *  int size_;
 */
class GeneralTemplate : noncopyable {
public:

    GeneralTemplate() : data_(nullptr), len_(0) {}
    GeneralTemplate(const char* data, int len)
        : data_(data),
          len_(len) {}

    const char* data_;
    int len_;
};
class LogStream : noncopyable{
public:
    // Small Buffer，默认大小4KB，用于存储一条log消息。
    using Buffer = FixedBuffer<kSmallBuffer>;

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }
    
    /**
     * 我们的LogStream需要重载运算符
     */
    // 这些运算符将内容写入LogStream中，即LogStream的buffer缓冲区中
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float);
    LogStream& operator<<(double);

    LogStream& operator<<(char);
    LogStream& operator<<(const void*);
    LogStream& operator<<(const char*);
    inline LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string& str);
    inline LogStream& operator<<(const Buffer& buf);

    //(const char*, int)的重载
    LogStream& operator<<(const GeneralTemplate& g);
    
private:
    static const int kMaxNumericSize = 48;

    //对整型需要特殊处理，格式化输出各种整型到LogStream的缓冲区中
    template <typename T>
    void formatInteger(T);

    // 用于存放log消息的Small Buffer
    Buffer buffer_;
};