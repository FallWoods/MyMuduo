#include"noncopyable.h"

class Logger{
public:
    enum LogLevel{
        TRACE=0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOGLEVELS
    };
};