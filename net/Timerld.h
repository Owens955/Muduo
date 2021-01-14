//
// Created by Kukai on 2020/12/14.
//

#ifndef MUDUO_NET_TIMERLD_H
#define MUDUO_NET_TIMERLD_H

namespace Kukai{

    class Timer;

    class Timerld{
    public:
        explicit Timerld(Timer *timer, int64_t seq = 0)
            : timer_(timer),
              sequence_(seq)
            { }

            friend class TimerQueue;

    private:
        Timer *timer_;
        int64_t sequence_;
    };
}

#endif //MUDUO_NET_TIMERLD_H
