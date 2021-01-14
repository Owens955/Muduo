//
// Created by Kukai on 2020/12/8.
//

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "Types.h"
#include <sys/unistd.h>

namespace Kukai{
    namespace CurrentThread{
        extern __thread int t_cachedTid;
        extern __thread const char* t_threadName;
        pid_t cacheTid();

        inline pid_t tid(){
            if(__builtin_expect(t_cachedTid == 0, 0)){
                cacheTid();
            }
            return t_cachedTid;
        }
        inline const char* name()
        {
            return t_threadName;
        }

        //bool isMainThread(){ return tid() == ::getpid(); }

    }
}

#endif //MUDUO_BASE_CURRENTTHREAD_H
