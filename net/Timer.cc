//
// Created by Kukai on 2020/12/14.
//

#include "Timer.h"

using namespace Kukai;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now) {
    if (repeat_){
        expiration_ = addTime(now, interval_);
    } else{
        expiration_ = Timestamp::invalid();
    }
}
