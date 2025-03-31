#include "LogFile.h"

LogFile::LogFile(const std::string& basename,
            off_t rollSize,
            int flushInterval,
            int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(std::make_unique<std::mutex>()),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0){
    // 重新启动时，可能没有log文件，因此在构建LogFile对象时，直接调用rollFile()以创建一个全新的日志文件。
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* data, int len) {
    std::lock_guard<std::mutex> lk(*mutex_);
    appendInLock(data, len);
}

void LogFile::appendInLock(const char* data, int len) {
    file_->append(data, len);
    // 判断是否需要滚动log
    if (file_->writtenBytes() > rollSize_) {
        rollFile();
    } else {
        ++count_;
        if (count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = ::time(NULL);
            // 计算现在是第几天
            // 计算现在是第几天 now/kRollPerSeconds会取整，求现在是第几天，忽略不满一天的时间，再乘以秒数相当于是当前天数0点对应的秒数
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            // 至少一天一滚
            if (thisPeriod != startOfPeriod_) {
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

void LogFile::flush() {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
}

// 滚动日志文件，rollFile只用专门负责如何滚动l，不需要判断是否应该滚动。
// 当日志文件接近指定的滚动限值（rollSize）时，需要换一个新文件写数据，便于后续归档、查看。调用LogFile::rollFile()可以实现文件滚动。
// 滚动日志时，原来的日志文件被关闭，打开一个新日志文件
// 日志文件名：basename + time + hostname + pid + ".log"
bool LogFile::rollFile() {
    time_t now = 0;
    // 获取新日志文件的名字，以及当前时间now
    std::string filename = getLogFileName(basename_, &now);
    // 计算现在是第几天 now/kRollPerSeconds求出现在是第几天，再乘以秒数相当于是当前天数0点对应的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    // time_t是整型，因此，最小的时间单元是1秒
    // 滚动操作会新建一个文件，而为避免频繁创建新文件，rollFile会确保上次滚动时间到现在如果不到1秒，就不会滚动。
    if (now > lastRoll_) {
        // to avoid identical roll by roll time
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        // 让file_指向一个名为filename的文件，相当于新建了一个文件，原来的FileUtil对象对销毁，关闭原来的日志文件
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}

// 取得唯一的log文件名
std::string LogFile::getLogFileName(const std::string& basename, time_t* now) {
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    // 结构化的时间表示
    struct tm tm_;
    *now = time(NULL);
    localtime_r(now, &tm_);
    // 写入时间
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm_);
    filename += timebuf;

    filename += ".log";

    return filename;
}
