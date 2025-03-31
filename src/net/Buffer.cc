#include "Buffer.h"

#include <cerrno>
#include <sys/uio.h>
#include <unistd.h>

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer(size_t initialSize)
    : buffer_(kCheapPrepend+initialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
{}

void Buffer::retrieve(size_t len){
    assert(len<=readableBytes());
    if(len<readableBytes()){
        readerIndex_+=len;
    }else{
        retrieveAll();
    }
}

int64_t Buffer::peekInt64() const {
    assert(readableBytes() >= 8);
    int64_t res;
    memcpy(&res,peek(),8);
    return be64toh(res);
}
int32_t Buffer::peekInt32() const {
    assert(readableBytes() >= 4);
    int32_t res;
    memcpy(&res,peek(),4);
    return be32toh(res);
}
int16_t Buffer::peekInt16() const {
    assert(readableBytes() >= 2);
    int16_t res;
    memcpy(&res,peek(),2);
    return be16toh(res);
}

int8_t Buffer::peekInt8()const{
    assert(readableBytes() >= 1);
    int8_t res=*peek();
    return res;
}

void Buffer::prepend(const void* data,size_t len){
    assert(prependableBytes()>=len);
    auto startPos=&buffer_[readerIndex_];
    memcpy(startPos-len,data,len);
}

void Buffer::shrink(size_t reserve){
    ensureWritableBytes(reserve);
    if(writeableBytes() > reserve){
        buffer_.resize(writerIndex_+reserve);
    }
}

void Buffer::makeSpace(size_t len){
    if(writeableBytes()+prependableBytes() < len+kCheapPrepend){
        // writable 空间大小 + prependable空间大小不足以存放len byte数据, resize内部缓冲区大小
        buffer_.resize(writerIndex_+len);
    }else{
        // writable 空间大小 + prependable空间大小 足以存放len byte数据, 移动readable空间数据, 合并多余prependable空间到writable空间
        assert(kCheapPrepend < readerIndex_);
        auto readSize=readableBytes();
        memmove(&*buffer_.begin()+kCheapPrepend,peek(),readSize);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readSize;
        assert(readSize == readableBytes());
    }
}

ssize_t Buffer::readFd(int fd, int& savedErrno){
    char extrabuf[65536]; //65536 = 64k bytes
    struct iovec vec[2];
    const size_t writable = writeableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const size_t n=sockets::readv(fd,vec,iovcnt);
    if(n < 0){
        // ::readv系统调用错误
        savedErrno=errno;
    }else if(n <= writable) {
        writerIndex_ += n;
    }else {
        // 读取的数据超过现有内部buffer_的writable空间大小时, 启用备用的extrabuf 64KB空间, 并将这些数据添加到内部buffer_的末尾
        // 过程可能会合并多余prependable空间或resize buffer_大小, 以腾出足够writable空间存放数据
        writerIndex_ = buffer_.size();
        append(extrabuf, n-writable);
    }
    return n;
}