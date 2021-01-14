//
// Created by 空海lro on 2020/12/21.
//

#include "Condition.h"

#include <errno.h>

bool Kukai::Condition::waitForSeconds(int seconds) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds;
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}
