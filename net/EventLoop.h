//
// Created by Kukai on 2020/12/7.
//

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "../base/Thread.h"
#include "../base/Mutex.h"
#include "../boost/noncopyable.h"
#include "../base/CurrentThread.h"
#include "Timerld.h"
#include "Callbacks.h"

#include <vector>
#include <memory>

namespace  Kukai{

    class Channel;
    class Poller;
    class TimerQueue;

class EventLoop : boost::noncopyable{
 public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();

    /*
     * eventloop核心函数，用于不断循环，在其中调用poller::poll()用于获取发生的事件
     * */
    void loop();

    /*
     * 停止loop
     * */
    void quit();

    /*
     * poller::poll返回时，得到其返回的事件
     * */
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    void runInLoop(const Functor &cb);

    void queueInLoop(const Functor &cb);

    /*
     * 在XXX时间戳执行cb
     * */
    Timerld runAt(const Timestamp &time, const TimerCallback &cb);
    /*
     * 在XXX秒之后执行cb
     * */
    Timerld runAfter(double delay, const TimerCallback &cb);
    /*
     * 每过XXX秒执行cb
     * */
    Timerld runEvery(double interval, const TimerCallback &cb);
    /*
     * 取消定时器timerld
     * */
    void cancel(Timerld timerld);

    void wakeup();

    /*
     * 更新channel，实际上调用了poller::updatechannel()，更新poller的pollfds数组
     * */
    void updateChannel(Channel* channel);

    void removeChannel(Channel *channel);

    /*
     * 断言，eventloop所属于的线程ID就是当前线程的ID
     * */
    void assertInLoopThread(){
        if(!isInLoopThread()){
            abortNotInLoopThread();
        }
    }
    /*
     * 判断是否eventloop所属于的线程ID就是当前线程ID
     * */
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid();}

private:
    /*
     * LOG_FATAL
     * */
    void abortNotInLoopThread();
    void handleRead();
    void doPendingFunctors();

    typedef  std::vector<Channel*> ChannelList;

    bool looping_;                            // eventloop是否正在loop
    bool quit_;                               // 是否退出
    bool callingPendingFunctors_;
    const pid_t threadId_;                    // eventloop所在线程ID，要求one eventloop one thread
    Timestamp pollReturnTime_;                // poller::poll返回时，得到其返回的事件
    std::unique_ptr<Poller> poller_;          // 用于在loop()中调用poller::poll()
    std::unique_ptr<TimerQueue> timerQueue_;  // 定时器队列
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;              // 当前活动的channel，用于handleEvent
    MutexLock mutex_;
    std::vector<Functor> pendingFuntors_;
};

}

#endif // MUDUO_NET_EVENTLOOP_H