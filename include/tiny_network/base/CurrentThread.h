#pragma once
#include<sys/syscall.h>
#include<unistd.h>

namespace CurrentThread{

// 保存tid缓冲，避免多次系统调用
extern __thread int t_cachedTid;
// extern thread_local char t_tidString[32];
// extern thread_local int t_tidStringLength;
// extern thread_local const char* t_threadName;

//获取线程的tid
void cachedTid();

inline int tid(){
    if(__builtin_expect(t_cachedTid==0,0)){
        //如果t_cachedTid==0，则获取线程的tid
        cachedTid();
    }
    //否则，直接返回
    return t_cachedTid;
}

// inline char* tidString(){ //for logging
//     return t_tidString;
// }

// inline int tidStringLength(){  //for logging
//     return t_tidStringLength;
// }

// inline const char* name(){
//     return t_threadName;
// }

// bool isMainThread();

// void sleepUsec(int64_t usec);//for logging

// std::string stackTrace(bool demangle);

};