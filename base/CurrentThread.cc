//
// Created by Kukai on 2020/12/8.
//

#include "CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace Kukai{
    namespace CurrentThread{
        __thread int t_cachedTid = 0;
        __thread const char* t_threadName = "unknown";
    }
}