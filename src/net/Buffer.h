#pragma once

#include <vector>
#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include "copyable.h"
#include "SocketsOps.h"


/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer: public copyable{
public:
    static const size_t kCheapPrepend=8;  //初始预留的prependable空间大小
    static const size_t kInitialSize=1024; // Buffer初始大小

    explicit Buffer(size_t initialSize = kInitialSize);

    size_t readableBytes()const{ /* 返回 readable 空间大小 */
        return writerIndex_-readerIndex_;
    }
    size_t writeableBytes()const{/* 返回 writeable 空间大小 */
        return buffer_.size()-writerIndex_;
    }
    size_t prependableBytes()const{/* 返回 writeable 空间大小 */
        return readerIndex_;
    }
    /* readIndex 对应元素地址，即待读数据的首地址*/
    const char* peek()const{
        return &buffer_[readerIndex_];
    }
    /* 返回待写入数据的首地址, 即writable空间首地址 */
    char* beginWrite(){
        return &buffer_[writerIndex_];
    }
    const char* beginWrite()const{
        return &buffer_[writerIndex_];
    }
    /* 返回缓冲区的起始位置, 也是prependable空间起始位置 */
    char* begin(){
        return &*buffer_.begin();
    }
    const char* begin()const{
        return &*buffer_.cbegin();
    }
    /* 返回buffer_的容量capacity() */
    size_t internalCapacity() const{
        return buffer_.capacity();
    }

    void swap(Buffer& other){
        buffer_.swap(other.buffer_);
        std::swap(other.writerIndex_,writerIndex_);
        std::swap(other.readerIndex_,readerIndex_);
    }

    /* 将writerIndex_往后移动len byte, 需要确保writable空间足够大 */
    void hasWritten(size_t len){
        assert(len<=writeableBytes());
        writerIndex_+=len;
    }
    /* 将writerIndex_往前移动len byte, 需要确保readable空间足够大 
     * Cancel written bytes.
     */
    void unwrite(size_t len){
        assert(len<=readableBytes());
        writerIndex_-=len;
    }
    /* 从readable头部取走最多长度为len byte的数据. 会导致readable空间变化, 可能导致writable空间变化.
     * 这里取走只是移动readerIndex_, writerIndex_, 并不会直接读取或清除readable, writable空间数据
     * retrieve() returns void, to prevent
     * string str(retrieve(readableBytes()), readableBytes());
     * the evaluation of two functions are unspecified
     */
    void retrieve(size_t len);
    /* 从readable空间取走 [peek(), end)这段区间数据, peek()是readable空间首地址 */
    void retriUntil(const char* end){
        assert(peek()<=end);
        retrieve(end-peek());
    }
    /* 从readable空间取走一个int64_t数据, 长度8byte */
    void retrieveInt64(){
        retrieve(8);
    }
    /* 从readable空间取走一个int32_t数据, 长度4byte */
    void retrieveInt32(){
        retrieve(4);
    }
    /* 从readable空间取走一个int16_t数据, 长度2byte */
    void retrieveInt16(){
        retrieve(2);
    }
    /* 从readable空间取走一个int8_t数据, 长度1byte */
    void retrieveInt8(){
        retrieve(1);
    }
    /* 从readable空间取走所有数据, 直接移动readerIndex_, writerIndex_指示器即可 */
    void retrieveAll(){
        readerIndex_=kCheapPrepend;
        writerIndex_=readerIndex_;
    }
    
    /* 从readable空间头部取走长度len byte的数据, 转换为字符串返回 */
    std::string retrieveAsString(size_t len){
        auto readStart=peek();
        retrieve(len);
        return std::string(readStart,len);
    }
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }
    //readInt系列函数从readable空间读取指定长度（类型）的数据，不仅从readable空间读取数据，
    //还会利用相应的retrieve函数把数据从中取走，导致readable空间变小。
    /* 从readable空间头部读取一个int64_类型数, 由网络字节序转换为本地字节序
     * Read int64_t from network endian
     *
     * Require: buf->readableBytes() >= sizeof(int64_t)
     */
    int64_t readInt64(){
        int64_t res = peekInt64();
        retrieveInt64();
        return res;
    }
    int32_t readInt32(){
        int32_t res = peekInt32();
        retrieveInt32();
        return res;
    }
    int16_t readInt16(){
        int16_t res = peekInt16();
        retrieveInt16();
        return res;
    }
    int8_t readInt8(){
        int8_t res = peekInt8();
        retrieveInt8();
        return res;
    }
    /* 从readable的头部peek()读取一个int64_t数据, 但不移动readerIndex_, 不会改变readable空间
     * Peek int64_t from network endian
     *
     * Require: buf->readableBytes() >= sizeof(int64_t)
     */
    int64_t peekInt64()const;
    int32_t peekInt32()const;
    int16_t peekInt16()const;
    /* 从readable的头部peek()读取一个int8_t数据, 但不移动readerIndex_, 不会改变readable空间.
     * 1byte数据不存在字节序问题。字节序指的是不同字节之间如何排序，字节内部的顺序不变 */
    int8_t peekInt8()const;
    // prepend系列函数预置指定长度数据到prependable空间。
    void prependInt64(int64_t x){
        int res=htobe64(x);
        prepend(&res,sizeof res);
    }
    void prependInt32(int32_t x){
        int res=htobe32(x);
        prepend(&res,sizeof res);
    }
    void prependInt16(int16_t x){
        int res=htobe16(x);
        prepend(&res,sizeof res);
    }
    void prependInt8(int8_t x){
        prepend(&x,sizeof x);
    }
    /* 在prependable空间末尾预置一组二进制数据data[len].
     * 表面上看是加入prependable空间末尾, 实际上是加入readable开头,　会导致readerIndex_变化 */
    void prepend(const void* data,size_t len);

    /* 收缩缓冲区空间, 将缓冲区中数据拷贝到新缓冲区, 确保writable空间最终大小为reserve */
    void shrink(size_t reserve);
    /* writable空间不足以写入len byte数据时,
     * 1)如果writable空间 + prependable空间不足以存放数据, 就resize 申请新的更大的内部缓冲区buffer_
     * 2)如果足以存放数据, 就将prependable多余空间腾挪出来, 合并到writable空间 */
    void makeSpace(size_t len);

    void ensureWritableBytes(size_t len){
        if(writeableBytes() < len){
            makeSpace(len);
        }
        assert(writeableBytes() >= len);
    }

    void append(const std::string& str){
        append(str.data(),str.size());
    }
    void append(char* data, size_t len){
        append(static_cast<const char*>(data), len);
    }
    void append(const char* data, size_t len){
        ensureWritableBytes(len);
        memcpy(beginWrite(), data, len);
        hasWritten(len);
    }
    void appendInt64(int64_t x){
        x = htobe64(x);
        append(reinterpret_cast<const char*>(&x),sizeof x);
    }
    void appendInt32(int32_t x){
        x = htobe32(x);
        append(reinterpret_cast<const char*>(&x),sizeof x);
    }
    void appendInt16(int16_t x){
        x = htobe16(x);
        append(reinterpret_cast<const char*>(&x),sizeof x);
    }
    void appendInt8(int8_t x){
        append(reinterpret_cast<const char*>(&x),sizeof x);
    }

    std::string toString() const{
        return std::string(peek(),readableBytes());
    }

    /**
    * 从fd读取数据到内部缓冲区, 将系统调用错误保存至savedErrno
    * @param 要读取的fd, 通常是代表连接的conn fd
    * @param savedErrno[out] 保存的错误号
    * @return 读取数据结果. < 0, 发生错误; >= 成功, 读取到的字节数
    */
    ssize_t readFd(int fd, int& savedErrno);

    /* 在readable空间找CRLF位置, 返回第一个出现CRLF的位置 */
    // CR = '\r', LF = '\n'
    const char* findCRLF() const{
        // FIXME: replace with memmem()?
        const char* crlf = std::search(peek(),beginWrite(),kCRLF,kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    /* 在start~writable首地址 之间找CRLF, 要求start在readable地址空间中 */
    const char* findCRLF(const char* start) const{
        assert(peek() <= start);
        assert(beginWrite() > start);
        // FIXME: replace with memmem()?
        const char* crlf=std::search(start,beginWrite(),kCRLF,kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    /* 在readable空间中找EOL, 即LF('\n') */
    // EOL = '\n'
    // find end of line ('\n') from range [peek(), end)
    const char* findEOL() const{
        const void* eol=memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }
    /* 在start~writable首地址 之间找EOL, 要求start在readable地址空间中 */
    // EOL = '\n
    // find end of line ('\n') from range [start(), end)
    // @require peek() < start
    const char* findEOL(const char* start) const{
        assert(peek() <= start);
        assert(beginWrite() > start);
        const void* eol=memchr(start, '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

private:
    std::vector<char> buffer_;  //存储数据的线性缓冲区，大小可变
    size_t readerIndex_;  //可读数据首地址
    size_t writerIndex_;  //可写数据首地址

    static const char kCRLF[];
};