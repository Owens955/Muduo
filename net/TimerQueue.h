//
// Created by Kukai on 2020/12/14.
//

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "../boost/noncopyable.h"
#include "../base/Timestamp.h"
#include "../base/Thread.h"
#include "Callbacks.h"
#include "Channel.h"

namespace Kukai{

    class EventLoop;
    class Timer;
    class Timerld;

    class TimerQueue : boost::noncopyable {
    public:
        /*
         * 隐式类型转换，loop为timerqueue所属于的那个eventloop
         * */
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();

        /*
         * 添加定时器到定时器集合中去，供eventloop使用来封装runAt()、runAfter()......
         * cb：回调函数
         * when：调用cb的时间
         * internal：第一次调用cb后，每隔internal时间再调用cb
         * */
        Timerld addTimer(const TimerCallback &cb, Timestamp when, double interval);

        void cancel(Timerld timerld);
    private:
        /*
         * 为了防止时间相同所导致的Key相同的情况，使用set和pair
         * */
        typedef std::pair<Timestamp, Timer*> Entry; // <时间戳，定时器指针>对
        typedef std::set<Entry> TimerList;          // multiset，定时器集合
        typedef std::pair<Timer*, int64_t> ActiveTimer;
        typedef std::set<ActiveTimer> ActiveTimerSet;

        void addTimerInLoop(Timer *timer);

        void cancelInLoop(Timerld timerld);

        /*
         * 定时器触发时调用
         * */
        void handleRead();

        /*
         * 获取所有已过期的定时器集合
         * */
        std::vector<Entry> getExpired(Timestamp now);

        /*
         * 重设已过期的定时器集合
         * */
        void reset(const std::vector<Entry> &expired, Timestamp now);

        /*
         * 插入一个定时器
         * */
        bool insert(Timer * timer);

        /*
         * 数据成员
         * */
        EventLoop *loop_;           //所属于的那个eventloop
        const int timerfd_;          //timerfd，关联channel注册可读事件
        Channel timerfdChannel_;    //与timerfd关联，发生可读事件就执行timer::run();
        TimerList timers_;          //定时器集合

        bool callingExpiredTimers_;
        ActiveTimerSet activeTimers_;
        ActiveTimerSet cancelingTimers_;
    };
}

#endif // MUDUO_TIMERQUEUE_H
