#include "Logging.h"
#include "CurrentThread.h"

namespace ThreadInfo{
    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;
};

const char* getErrnoMsg(int savedErrno){
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof ThreadInfo::t_errnobuf);
}

//根据Level返回Level的名字
const char* getLevelName[Logger::LogLevel::NUM_LOGLEVELS] = { "TRACE ", "DEBUG ", "INFO  ",
                                                                "WARN  ", "ERROR ", "FATAL "};

Logger::LogLevel initLogLevel() {
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

// 默认的日志输出函数，即日志默认被输出到终端中
static void defaultOutput(const char* data, int len) {
    fwrite(data, len, sizeof(char), stdout);
}
// 默认刷新函数
static void defaultFlush(){
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const char* file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file) {
    // 把当前时间写入日志流
    formatTime();
    // 写入日志等级
    stream_ << GeneralTemplate(getLevelName[level], 6);
    // TODO:error
    if (savedErrno != 0){
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// Timestamp::toString方法的思路，只不过这里需要输出到流
void Logger::Impl::formatTime() {
    Timestamp now = Timestamp::now();
    // time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    // int microSeconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

    // struct tm* tm_time = localtime(&seconds);
    // //写入线程存储的时间buf中
    // int n = snprintf(ThreadInfo::t_time, sizeof tm_time, "%4d/%02d/%02d %02d:%02d:%02d:",
    //     tm_time->tm_year + 1900,
    //     tm_time->tm_mon + 1,
    //     tm_time->tm_mday,
    //     tm_time->tm_hour,
    //     tm_time->tm_min,
    //     tm_time->tm_sec);
    // 更新最后一次调用发生的时间
    ThreadInfo::t_lastSecond = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    // 写入线程存储的时间buf中
    snprintf(ThreadInfo::t_time, sizeof ThreadInfo::t_time, now.toFormattedString().c_str());
    //把微妙数转换成字符串形式
    // char buf[32] = {0};
    // snprintf(buf, sizeof buf, "%06d ",microSeconds);
    //输出时间，附有微妙(之前是(buf, 6),少了一个空格)
    stream_ << now.toFormattedString();
}

// 添加后缀：文件名、行数
void Logger::Impl::finish() {
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_)
    << ":" << line_ << '\n';
}

// level默认为INFO等级，file指的是所在源代码的文件，用__FILE__宏赋值
Logger::Logger(const char* file, int line)
    : impl_(INFO, 0, file, line){}

Logger::Logger(const char* file, int line, Logger::LogLevel level)
    : impl_(level, 0, file, line){}

Logger::Logger(const char* file, int line, Logger::LogLevel level, const char* func)
    : impl_(level, 0, file, line){
    impl_.stream_ << func << ' ';
}

Logger::~Logger() {
    // 往Small Buffer添加后缀 文件名:行数
    impl_.finish();
    // 获取buffer(stream_.buffer_)
    const LogStream::Buffer& buf(stream().buffer());
    // 将LogStream的缓冲区中的内容输出(默认向终端输出)，如果想要将日志输出到文件，只需要重新设置g_output函数，设置为AsyncLogging的append()函数
    g_output(buf.data(), buf.length());
    // FATAL情况终止程序
    if (impl_.level_ == FATAL) {
        // 发生致命错误, 程序异常终止，需要手动刷新缓冲区
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}