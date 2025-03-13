#include "CurrentThread.h"

namespace CurrentThread{

__thread int t_cachedTid = 0;
// thread_local char t_tidString[32];
// thread_local int t_tidStringLength;
// thread_local const char* t_threadName;

void cachedTid() {
    if (t_cachedTid == 0) {
        //通过系统调用获取当前线程的tid值
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}

};