#pragma once

#include <stdio.h>
#include <string>


// 采用RAII方式管理文件资源，构建对象即打开文件，销毁对象即关闭文件。
class FileUtil {
public:
    explicit FileUtil(const std::string& fileName);
    ~FileUtil();

    // 添加log消息到文件末尾
    void append(const char* data, size_t len);
    // 冲刷文件内容到磁盘
    void flush() {
        ::fflush(fp_);
    }
    // 返回已写字节数
    off_t writtenBytes() const { return writtenBytes_; }
private:
    // 写数据到文件
    size_t write(const char* data, size_t len) {
        // write通过非线程安全的glibc库函数fwrite_unlocked()来完成写文件操作，
        // 而没有选择线程安全的fwrite()，主要是出于性能考虑。
        /**
         * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
         * -- buffer:指向数据块的指针
         * -- size:每个数据的大小，单位为Byte(例如：sizeof(int)就是4)
         * -- count:数据个数
         * -- stream:文件指针
         */
        return ::fwrite_unlocked(data, 1, len, fp_);
    }
    // 文件指针
    FILE* fp_;
    char buffer_[64 * 1024];  // fp_的缓冲区
    // off_t用于指示文件的偏移量，表示已经写了多少字节
    off_t writtenBytes_;
};