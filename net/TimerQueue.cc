//
// Created by 空海lro on 2020/12/14.
//

#define __STDC_LIMIT_MACROS
#include "TimerQueue.h"

#include "EventLoop.h"
#include "Timer.h"
#include "Timerld.h"
#include "../base/Log.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <algorithm>
#include <functional>

namespace Kukai{
    namespace detail{

        /*
         * 内部使用的函数，创建一个timerfd，用于产生定时器事件：在poller::poll()获取定时器事件信息
         * */
        int createTimerfd(){
            int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
            if (timerfd < 0){
                LOG("Failed in timerfd_create");
            }
            return timerfd;
        }

        /*
         * 从现在到when这个时间戳还有多长时间，用timespec表示
         * */
        struct timespec howMuchTimeNow(Timestamp when){

            // 剩余微秒数
            int64_t microseconds = when.microSecondsSinceEpoch()
                                   - Timestamp::now().microSecondsSinceEpoch();
            if (microseconds < 100){
                microseconds = 100;
            }

            // 设置秒和毫秒
            struct timespec ts;
            ts.tv_sec = static_cast<time_t>(
                    microseconds / Timestamp::kMicroSecondsPerSecond);
            ts.tv_nsec = static_cast<long>(
                    (microseconds % Timestamp::kMicroSecondsPerSecond) *1000);
            return ts;
        }

        /*
         * 从timerfd这个文件描述符上读一个unit64_t
         * */
        void readTimerfd(int timerfd, Timestamp now){
            uint64_t howmany;
            ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
            LOG("TimerQueue:: = handleRead() " << howmany << " at " << now.toString());
            if (n != sizeof howmany){
                LOG("TimerQueue::handleRead() reads " << n << " bytes instead of 8 ");
            }
        }

        /*
         * 重置timerfd
         * */
        void resetTimerfd(int timerfd, Timestamp expiration){
            struct itimerspec newValue;
            struct itimerspec oldValue;
            bzero(&newValue, sizeof newValue);
            bzero(&oldValue, sizeof oldValue);
            newValue.it_value = howMuchTimeNow(expiration);
            int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);

            if (ret){
                LOG("timerfd_settime() ");
            }
        }
    }
}

using namespace Kukai;
using namespace Kukai::detail;

/*
 * explicit隐式类型转换,loop为timerqueue所属于的那个eventloop
 * */
TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),        // 新建timerfd
      timerfdChannel_(loop, timerfd_),  // timerqueue对象成功构造后，timerfd关联到timerfdChannel上
      timers_(),
      callingExpiredTimers_(false){
         // 当timerfd上读事件发生时，channel回调timerqueue::handleRead()
         timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
         // timerfdchannel注册读事件，用于接收定时器事件
         timerfdChannel_.enableReading();
}

/*
 * 定时器结束后，禁止定时器channel的所有网络事件，并移除它
 * */
TimerQueue::~TimerQueue() {
    ::close(timerfd_);  // 清空TimerList，由于timers_是个指针，应当回收内存
    for (TimerList::iterator it = timers_.begin();
        it != timers_.end(); ++it) {
        delete it->second;
    }
}

/*
 * 创建一个定时器,获取其ID,会让eventloop调用addTimerInLoop方法添加一个定时器
 * */
Timerld TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, double interval) {
    //创建一个新的timer，回调为TimerCallback，在when时触发定时器事件，每隔interval触发一次
    Timer *timer = new Timer(cb, when, interval);
    //在eventloop中回调timerqueue::addTimerInLoop 用于把新建的定时器timer添加到eventloop中
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

    return Timerld(timer);  //返回创建的定时器
}

void TimerQueue::cancel(Timerld timerld){
    loop_->runInLoop(
            std::bind(&TimerQueue::cancelInLoop, this, timerld));
}

void TimerQueue::addTimerInLoop(Timer *timer) {

    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(Timerld timerld){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerld.timer_, timerld.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first;
        activeTimers_.erase(it);
    } else if (callingExpiredTimers_){
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

/*
 * 当timerfd可读网络事件触发时调用
 * */
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    //获取过期的定时器集合
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    //过期的timer开始在channel中执行
    for (std::vector<Entry>::iterator it = expired.begin() ; it != expired.end() ; ++it) {
        it->second->run();  // timer::run()，执行创建timer时绑定的回调函数
    }
    callingExpiredTimers_ = false;

    reset(expired, now); // 重设过期时间
}

/*
 * 从timers中移除已到期的timer，并通过vector返回这些过期的timer
 * */
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    for (const Entry &it : expired){
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

/*
 * 重置已过期的定时器集合
 * */
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
    Timestamp nextExpire;

    for(std::vector<Entry>::const_iterator it = expired.begin() ; it != expired.end() ; ++it){
        ActiveTimer  timer(it->second, it->second->sequence());
        if (it->second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end()){
            it->second->restart(now);
            insert(it->second);
        } else{
            delete it->second;
        }
    }

    if (!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid()){
        resetTimerfd(timerfd_, nextExpire);
    }
}

/*
 * 插入一个定时器
 * */
bool TimerQueue::insert(Timer *timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first){
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(std::make_pair(when, timer));
        assert(result.second);
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
            = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}