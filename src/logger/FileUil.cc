#include "FileUtil.h"
#include "Logging.h"

FileUtil::FileUtil(const std::string& fileName)
    : fp_(::fopen(fileName.c_str(), "ae")),  // 'e' for O_CLOEXEC
      writtenBytes_(0) {
    // 自行设置文件流的缓冲区
    ::setvbuf(fp_, buffer_, _IOFBF, sizeof buffer_);
}

FileUtil::~FileUtil(){
    flush();
    ::fclose(fp_);
}

// 向文件写入数据
void FileUtil::append(const char* data, size_t len){
    // 记录已经写入的数据大小
    size_t written = 0;

    while (written != len) {
        // 还需写入的大小
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
                // clear error indicators for fp_
                clearerr(fp_);
                break;
            }
        }
        // 更新写入的数据的大小
        written += n;
    }
    // 记录目前为止写入的数据大小，超过限制会滚动日志
    writtenBytes_ += written;
}