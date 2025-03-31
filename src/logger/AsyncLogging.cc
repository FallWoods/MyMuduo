#include "AsyncLogging.h"
#include "Timestamp.h"

#include <stdio.h>

AsyncLogging::AsyncLogging(const std::string& basename,
                            off_t rollSize,
                            int flushInterval)
    : flushInterval_(flushInterval),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this)),
      currentBuffer_(std::make_unique<Buffer>()),
      nextBuffer_(std::make_unique<Buffer>()){
    buffers_.reserve(16);
}

AsyncLogging::~AsyncLogging() {
    if (running_) {
        stop();
    }
}

// 前端线程通过调用此函数将log消息传递到后端
// append()可能会被多个前端线程调用，因此必须考虑线程安全，用mutex_加锁。
void AsyncLogging::append(const char* logline, int len) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (currentBuffer_->avail() > len) {
        // 缓冲区剩余空间足够则直接写入
        currentBuffer_->append(logline, len);
    } else {
        // 当前缓冲区空间不够，将新信息写入备用缓冲区
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            // 备用缓冲区也不够时，重新分配缓冲区
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        // 唤醒写入磁盘的后端线程，此时，至少有一个已满的缓冲区可以写到磁盘
        cond_.notify_one();
    }
}

// 后端线程运行的函数
void AsyncLogging::threadFunc() {
    // output有写入磁盘的接口
    LogFile output(basename_, rollSize_, false);
    // 后端缓冲区，用于归还currentBuffer,nextBuffer
    BufferPtr newBuffer1(std::make_unique<Buffer>());
    BufferPtr newBuffer2(std::make_unique<Buffer>());
    // 后端缓冲区数组，用于和前端缓冲区数组进行交换
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            // 1）每次当已满缓冲队列中有数据时，或者即使没有数据，但3秒超时，就将当前缓冲加入到已满缓冲队列（即使当前缓冲没满），
            //    将buffer1移动给当前缓冲，buffer2移动给空闲缓冲（如果空闲缓冲已移动的话）
            if (buffers_.empty()) {
                //等待3秒
                cond_.wait_for(lk, std::chrono::seconds(3));
            }
            // 此时正使用的buffer也放入已满缓冲数组中（没写完也放进去，避免等待太久才刷新一次）
            buffers_.push_back(std::move(currentBuffer_));
            // 归还正在使用的缓冲区
            currentBuffer_ = std::move(newBuffer1);
            // 进行交换，获取已满缓冲区数组的管理权
            buffersToWrite.swap(buffers_);
            // 如果备用空闲缓冲区已被移动，则重新分配它
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        // 遍历所有buffer，将其写入log文件
        for (const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }

        // 只保留两个缓冲区
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        // 归还newBuffer1
        if (!newBuffer1) {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        // 归还newBuffer2
        if (!newBuffer2) {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        // 清空后端缓冲区队列
        buffersToWrite.clear();
        // 内核高速缓存中的数据flush到磁盘，防止意外情况造成数据丢失。
        output.flush();
    }
}