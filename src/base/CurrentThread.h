#pragma once
#include<sys/syscall.h>
#include<unistd.h>

namespace CurrentThread{

// 保存tid缓冲，避免多次系统调用
extern __thread int t_cachedTid;

//获取线程的tid
void cachedTid();

inline int tid(){
    if(__builtin_expect(t_cachedTid == 0, 0)) {
        //如果t_cachedTid==0，则获取线程的tid
        cachedTid();
    }
    //否则，直接返回
    return t_cachedTid;
}

};